#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"leptjson.h"
#include <assert.h>  // assert()

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

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect)==(actual),expect, actual, "%d")

static void test_parse_null() {
	lept_value v;
	v.type = LEPT_FALSE;
	// 尝试解析"null"，解析成功，类型为LEPT_NULL
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null")); 
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v)); 
}

static void test_parse_true() {
	lept_value v;
	v.type = LEPT_FALSE;
	// 尝试解析"true"，解析成功，类型为LEPT_TRUE
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true")); 
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v)); 
}

static void test_parse_false() {
	lept_value v;
	v.type = LEPT_TRUE;
	// 尝试解析"false"，解析成功，类型为LEPT_FALSE
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false")); 
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v)); 
}

static void test_parse_expect_value() {
	lept_value v;
	v.type = LEPT_FALSE;
	// 尝试解析""空白，解析结果是LEPT_PARSE_EXPECT_VALUE（解析结果正确，但这种应该非法（或者说是字符串））,类型为LEPT_NULL
	EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, "")); 
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));               
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, " "));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_invalid_value() {
	lept_value v;
	v.type = LEPT_FALSE;
	// 尝试解析"nul"，"?"等无效值空白，解析结果是LEPT_PARSE_INVALID_VALUE, 类型为LEPT_NULL
	EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "nul")); 
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));                    
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "?")); 
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v)); // 
}

static void test_parse_root_not_singular() {
	lept_value v;
	v.type = LEPT_FALSE;
	// 尝试解析"null x", "true x"等无效值，解析结果应该为LEPT_PARSE_ROOT_NOT_SINGULAR, 但类型应该为LEPT_NULL
	// 这里他的判断逻辑，解析这种正确值后面还跟有非空字符的，把类型归为正确值的类型，我个人觉得不对，应该均为LEPT_NULL
	// 否则"true x"为LEPT_TRUE，显然不对
	// 修正lept_parse逻辑，检测到解析结果为LEPT_PARSE_ROOT_NOT_SINGULAR时，类型设置为LEPT_NULL（非法）
	EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x")); 
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));                           

	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "true x")); 
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));                           
}

static void test_parse() {
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
}

int main() {
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
	return main_ret;
}
