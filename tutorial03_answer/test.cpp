#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"leptjson.h"
#include <crtdbg.h>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)\
	do {\
		test_count++;\
        printf("%d\n", test_count);\
		if (equality)\
			test_pass++; \
		else {\
		    fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual); \
		    main_ret = 1; \
		}\
	}while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect)==(actual),expect, actual, "%d")  // ������ͣ����������json���ͣ�����Ϊint��
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect)==(actual),expect, actual, "%��17g") // ���json����ֵ
#define EXPECT_EQ_STRING(expect, actual, alength)\
	EXPECT_EQ_BASE(sizeof(expect)-1==alength&&memcmp(expect,actual,alength)==0,expect,actual,"%s")   // ���json�ַ���ֵ
// sizeof(expect)-1 == lept_get_string_length(&v)
// �� ptr1 ��ָ����ڴ���ǰ num ���ֽ��� ptr2 ָ��ĵ�һ���ֽ������бȽϣ�
// ������Ƕ�ƥ�䣬�򷵻��㣬���������ƥ�䣬�򷵻����㲻ͬ��ֵ����ʾ�ĸ�ֵ����
// ��ע�⣬��strcmp��ͬ���ú������ҵ����ַ��󲻻�ֹͣ�Ƚϡ�
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual)!=0, "true", "false", "%s")     // ��������ǲ���true
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual)==0, "false", "true", "%s")

static void test_parse_null() {
	lept_value v;
	lept_init(&v);    // (v)->type=LEPT_NULL �� v����ָ��NULL
	lept_set_boolean(&v, 0);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null")); 
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v)); 
	lept_free(&v);
}

static void test_parse_true() {
	lept_value v;
	lept_init(&v);
	lept_set_boolean(&v, 0);  // v->type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true")); 
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v)); 
	// EXPECT_TRUE(lept_get_boolean(&v));  // EXPECT_EQ_INT ����expectֵ��actualֵ�Ƚ��ж��Ƿ������ȷ�����ݵ�����������
	                                    // EXPECT_TRUEֱ�ӱȽ϶���Ĳ���ֵ������ǰ�ж��¶����ǲ��ǲ������ͣ�
	                                    // assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
	lept_free(&v);
}

static void test_parse_false() {
	lept_value v;
	lept_init(&v);  
	lept_set_boolean(&v, 1);    // // v->type = LEPT_TRUE
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false")); 
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v)); 
	// EXPECT_FALSE(lept_get_boolean(&v));
	lept_free(&v);
}

#define TEST_NUMBER(expect, json)\
	do{\
		lept_value v;\
		lept_init(&v);\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,json));\
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
		EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
		lept_free(&v);\
	}while(0)

static void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

	TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");

}

#define TEST_STRING(expect, json)\
	do{\
		lept_value v; \
		lept_init(&v);\
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,json));\
		EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
		EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
		lept_free(&v);\
	}while(0)

static void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");

	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\""); 
	//TEST_STRING("/", "\"\\/\"");
	//TEST_STRING("\/", "\"\\/\"");
	//TEST_STRING("\/", "\"\/\"");
	//TEST_STRING("\\/", "\"\\/\"");
	//lept_value v; 
	//lept_init(&v);
	//lept_parse(&v, "\"\\/\"");
	//printf("epect��%s��actual��%s\n", "/", lept_get_string(&v));
	//lept_free(&v);
	//lept_parse(&v, "\"\/\"");
	//printf("epect��%s��actual��%s\n", "/", lept_get_string(&v));
	//lept_free(&v);
	//lept_parse(&v, "\"\\/\"");
	//printf("epect��%s��actual��%s\n", "\/", lept_get_string(&v));
	//lept_free(&v);
	//lept_parse(&v, "\"\/\"");
	//printf("epect��%s��actual��%s\n", "\/", lept_get_string(&v));
	//lept_free(&v);
	//epect�� / ��actual�� /
	//epect�� / ��actual�� /
	//epect�� / ��actual�� /
	//epect�� / ��actual�� /
}
// "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\""
// "\\\" \\\\ \\/ \\b \\f \\n \\r \\t"
// "\" \\ \/ \b \f \n \r \t"


// �ع�����
#define TEST_ERROR(error, json)\
	do{\
		lept_value v;\
		v.type = LEPT_FALSE;\
		EXPECT_EQ_INT(error, lept_parse(&v, json));\
		EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
	}while(0)

static void test_parse_expect_value() {
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");

	/* invalid number */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");

}

static void test_parse_root_not_singular() {
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

	/* invalid number */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");      // ȱ�����ţ�Ӧ����"\"\""->""
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");   //  "\"abc\""->"abc"              
}

static void test_parse_invalid_string_escape() {           // ��Ч���ַ���ת��
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");   //"\v"���ǺϷ���json�ַ���
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");   // "\'"���ǺϷ���json�ַ���
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");   // "\0" ���Ϸ���json���ϣ���"\u0000"��json���ַ�������Ҫ\0��Ϊ��β
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");  // "\x12"���Ϸ�
}

static void test_parse_invalid_string_char() {              // ��Ч���ַ�ֵ�� 0 �� 31��",\��
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

// ���ʵĵ�Ԫ����
static void test_access_null() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);  // �ַ�������null���������壬��Ҫ��֤lept_free��û���ͷ��ַ����ڴ�
	lept_set_null(&v);
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
	lept_free(&v);
}

static void test_access_boolean() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_boolean(&v, 1);
	EXPECT_TRUE(lept_get_boolean(&v));
	lept_set_boolean(&v, 0);
	EXPECT_FALSE(lept_get_boolean(&v));
	lept_free(&v);
}

static void test_access_number() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_number(&v, 1234.5);
	EXPECT_EQ_DOUBLE(1234.5, lept_get_number(&v));
	lept_free(&v);
}

static void test_access_string() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
	lept_set_string(&v, "Hello", 5);
	EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
	lept_free(&v);
}

static void test_parse() {
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_number();
	test_parse_string();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
	test_parse_number_too_big();
	test_parse_missing_quotation_mark();
	test_parse_invalid_string_escape();
	test_parse_invalid_string_char();

	test_access_null();
	test_access_boolean();
	test_access_number();
	test_access_string();
}

int main() {
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	return main_ret;
}