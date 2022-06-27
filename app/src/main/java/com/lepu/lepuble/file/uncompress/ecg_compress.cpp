#include "ecg_compress.h"

//************************************************
//��ͨ��ѹ����ʽ
//ԭʼ���ݣ�ѹ�����(1Byte)+ԭʼ����
//��չ���ݣ���չ���(1Byte)+��չ���
//ѹ�����ݣ����ѹ������
//��ͨ��ѹ����ʽ
//ԭʼ���ݣ�ѹ�����(1Byte)+λ���(1Byte)+ԭʼ����
//ѹ�����ݣ����ѹ������
//************************************************


#define COM_RET_ORIGINAL	(-128)		//��Ҫ����ԭʼֵ����ֵ
#define	COM_RET_POSITIVE	(127)		//��Ҫ������չ��Ϊ��������ֵ
#define	COM_RET_NEGATIVE	(-127)		//��Ҫ������չ��Ϊ��������ֵ

#define	COM_MAX_VAL			(127)			//ѹ�����ֵ
#define	COM_MIN_VAL			(-127)			//ѹ����Сֵ
#define	COM_EXTEND_MAX_VAL	(382)			//ѹ����չ���ֵ
#define	COM_EXTEND_MIN_VAL	(-382)			//ѹ����չ��Сֵ

#define ECG_CHANNEL_MAX_NUM				8

static unsigned char m_channel_num;			//ͨ����


//��ѹ��ر���
static char m_uncompress_step;				//��ѹ����	0x00:���� 0x01~0x0F:��ͨ��  0x11~:��ͨ��

//************************************************************************
// ��������: ecg_uncompress_init 
//
// ����˵��: ECG��ֽ�ѹ�㷨��ʼ��
//
// �������: 
//			 unsigned char channel_num: ѹ������ͨ���� ����ʵ��ѹ�������� ���֧��8ͨ��
//
// �������: NULL
//
// ��    ʷ: Created by chenzhuangli : 2021/11/19
//************************************************************************
void ecg_uncompress_init(unsigned char channel_num)
{
	m_uncompress_step = 0;
	m_channel_num = channel_num;
	return;
}

//************************************************************************
// ��������: ecg_uncompress_alg 
//
// ����˵��: ECG��ֽ�ѹ����
//
// �������:signed char compress_data ѹ�����ݵ����ֽڲ���
//			unsigned char *p_output_data ԭʼ���ݵ������������ָ��
// �������: unsigned char:����ֵ ��ѹ���
//
// ��    ʷ: Created by chenzhuangli : 2021/11/19
//************************************************************************
unsigned char ecg_uncompress_alg(signed char compress_data, short *p_output_data)
{
	static short last_data[ECG_CHANNEL_MAX_NUM] = { 0 };		//���һ�ν�ѹ����
	static short uncompress_data[ECG_CHANNEL_MAX_NUM] = { 0 };	//��ǰ�������ѹ����  ��ͨ����Ч
	static unsigned char uncompress_len;				//��ǰ�������ѹ�����ѽ�ѹ���� ��ͨ����Ч
	unsigned char uncompress_ret = UNCOM_RET_INVALI;	//��ѹ����ֵ����ѹ���
	static unsigned char original_bitmask = 0x00;	//ԭʼ����λ���

	switch (m_uncompress_step) {
		case 0x00:			//�������ݽ���
			if (m_channel_num == 1) {		//��ͨ������				
				if (compress_data == COM_RET_ORIGINAL) {
					m_uncompress_step = 0x01;		//��һ������ԭʼ����
				}else if (compress_data == COM_RET_POSITIVE) {		//��
					m_uncompress_step = 0x03;
				}else if (compress_data == COM_RET_NEGATIVE) {		//��
					m_uncompress_step = 0x04;
				}else {
					p_output_data[0] = last_data[0] + compress_data;
					last_data[0] = p_output_data[0];
					uncompress_ret = UNCOM_RET_VALID;
				}
			}else {				//��ͨ������				
				if (compress_data == COM_RET_ORIGINAL) {
					m_uncompress_step = 0x11;		//��һ������ԭʼ����
					original_bitmask = 0x00;
					uncompress_len = 0;
				} else {
					uncompress_data[uncompress_len] = last_data[uncompress_len] + compress_data;
					last_data[uncompress_len] = uncompress_data[uncompress_len];
					if (++uncompress_len >= m_channel_num) {
						for (unsigned char i = 0; i < m_channel_num; i++) {
							p_output_data[i] = uncompress_data[i];
						}
						uncompress_len = 0;
						uncompress_ret = UNCOM_RET_VALID;
					}
				}
			}			
			break;
		case 0x01:			//ԭʼ�����ֽڵ�λ
			last_data[0] = (unsigned char)compress_data;
			m_uncompress_step = 0x02;
			break;
		case 0x02:			//ԭʼ�����ֽڸ�λ
			p_output_data[0] = last_data[0] + (compress_data << 8);
			last_data[0] = p_output_data[0];
			m_uncompress_step = 0x00;
			uncompress_ret = UNCOM_RET_VALID;
			break;
		case 0x03:
			p_output_data[0] = COM_MAX_VAL + (last_data[0] + (unsigned char)compress_data);
			last_data[0] = p_output_data[0];
			m_uncompress_step = 0x00;
			uncompress_ret = UNCOM_RET_VALID;
			break;
		case 0x04:
			p_output_data[0] = COM_MIN_VAL + (last_data[0] - (unsigned char)compress_data);
			last_data[0] = p_output_data[0];
			m_uncompress_step = 0x00;
			uncompress_ret = UNCOM_RET_VALID;
			break;
		case 0x11:
			original_bitmask = (unsigned char)compress_data;
			if (original_bitmask != 0xFF && original_bitmask) {
				uncompress_data[0] = 0;
			}
			m_uncompress_step = 0x12;
			break;
		case 0x12:		//ԭʼ�����ֽڵ�λ
			if (original_bitmask & (1 << uncompress_len)) {			//Ϊѹ�����������
				last_data[uncompress_len] = (unsigned char)compress_data;
				m_uncompress_step = 0x13;				
			} else {
				uncompress_data[uncompress_len] = last_data[uncompress_len] + compress_data;
				last_data[uncompress_len] = uncompress_data[uncompress_len];
				if (++uncompress_len >= m_channel_num) {
					for (unsigned char i = 0; i < m_channel_num; i++) {
						p_output_data[i] = uncompress_data[i];
					}
					m_uncompress_step = 0x00;
					uncompress_len = 0;
					uncompress_ret = UNCOM_RET_VALID;
				}
			}
			break;
		case 0x13:		//ԭʼ�����ֽڸ�λ			
			uncompress_data[uncompress_len] = last_data[uncompress_len] | (compress_data << 8);
			last_data[uncompress_len] = uncompress_data[uncompress_len];
			m_uncompress_step = 0x12;
			if (++uncompress_len >= m_channel_num) {
				for (unsigned char i = 0; i < m_channel_num; i++) {
					p_output_data[i] = uncompress_data[i];
				}
				m_uncompress_step = 0x00;
				uncompress_len = 0;
				uncompress_ret = UNCOM_RET_VALID;
			}
			break;
		default:
			break;
	}
	return uncompress_ret;
}