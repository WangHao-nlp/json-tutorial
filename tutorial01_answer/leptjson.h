#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum {
	LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LRPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT
} lept_type;
//      json���������ͣ������գ����������������ַ��������飬����
//null: ��ʾΪ null
//boolean : ��ʾΪ true �� false
//number : ������
//string : ��ʾΪ "abc"
//array : ��ʾΪ[null,true,number,string,[,,]]
//object : ��ʾΪ{"k1":null,"k2":true,"k3":123,"k4":"value4","k5":[1,2],"k6":{}}

typedef struct {
	lept_type type;
}lept_value;
// jsonֵ�����ڵ�һ��ֻ���պͲ�������������ֻ���������ͣ��պͲ������͵�ֵ��Ϊ���ǵ����ͣ�
// ����������ַ��������ݣ�����������ֵ������ĳ�Ա����С������������ĳ�Ա����С��������


// ���������������ǰ���¶���һ������⣬��һ�μ��ֻ�ܼ��һ�����ͣ����ܼ�⸴�ϵ�json���󣨵����½��ܣ�
// һ��ʼֻ�ܼ��"123"�����֣�"true"�ǲ�����"\"abc\""���ַ���
enum {
	LEPT_PARSE_OK = 0,                // �޴���
	LEPT_PARSE_EXPECT_VALUE,          // ��, �� " ",""
	LEPT_PARSE_INVALID_VALUE,         // ��Чֵ�����������null��true��false��
	LEPT_PARSE_ROOT_NOT_SINGULAR      // ������滹������ֵ��"null n"
};

int lept_parse(lept_value* v, const char* json);  // ���������������ַ�����������json����

lept_type lept_get_type(const lept_value* v);     
// ���ʽ�����������ֻ���ʶ������ͣ���"null"ΪLEPT_NULL��"123"ΪLRPT_NUMBER

#endif // !LEPTJSON_H__