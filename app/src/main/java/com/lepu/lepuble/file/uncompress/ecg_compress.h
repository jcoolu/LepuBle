#ifndef __ECG_COMPRESS_H__
#define	__ECG_COMPRESS_H__

#define	UNCOM_RET_INVALI	(0)		//��ѹ���账����ֵ
#define	UNCOM_RET_VALID		(1)		//��ѹ�����Ч����

void ecg_uncompress_init(unsigned char channel_num);
unsigned char ecg_uncompress_alg(signed char compress_data, short *p_output_data);

#endif // !__ECG_COMPRESS_H__
