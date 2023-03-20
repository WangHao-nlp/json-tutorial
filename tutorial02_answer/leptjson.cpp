#include "leptjson.h"
#include <assert.h>    // assert()
#include <errno.h>     // errno, ERANGE
#include <math.h>      // HUGE_VAL
#include <stdlib.h>    // NULL strtod()

#define EXPECT(c, ch) do{assert(*c->json==(ch));c->json++;}while(0)
#define ISDIGIT(ch) ((ch)>='0'&&(ch)<='9')
#define ISDIGIT1TO9(ch) ((ch)>='1'&&(ch)<='9')

typedef struct {
	const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {   
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	c->json = p; 
}

// 重构解析null、true、false
// literal用来判断是不是null|true|false，type用来解析完给v复制
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 0; literal[i + 1]; i++) {
		if (c->json[i] != literal[i + 1]) {
			// 因为v.type初始化为LEPT_NULL，所以解析为无效值后，这里不用重新给type赋值
			return LEPT_PARSE_INVALID_VALUE;
		}
	}
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	//// 版本1
	/*
	char* end;
	v->n = strtod(c->json, &end);  
	// C 库函数 double strtod(const char *str, char **endptr) 
	// 把参数 str 所指向的字符串转换为一个浮点数（类型为 double 型）。
	// 如果 endptr 不为空，则指向转换中最后一个字符后的字符的指针会存储在 endptr 引用的位置。
	if (c->json == end) // 即无数字部分
		return LEPT_PARSE_INVALID_VALUE;
	c->json = end;  // 有数字部分，但可能数字后还有其他字符
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
	*/

	// 版本2
	const char* p = c->json;
	if (*p == '-')p++;   // 先检查有没有负号（不可以有＋号开头）
	if (*p == '0') {     // 再检查整数部分，是不是0或者1-9开头的整数
		p++;
	}
	else {
		if (!ISDIGIT1TO9(*p)) {
			return LEPT_PARSE_INVALID_VALUE;  // 开头既不是0也不是1-9,则是无效值
		}
		for (p++; ISDIGIT(*p); p++);   //开头是123..9，然后忽略中间的数字，如0-9，直接跳过整数部分
	}
	if (*p == '.') {                     // 检查是否有小数点
		p++;
		if (!ISDIGIT(*p)) {
			return LEPT_PARSE_INVALID_VALUE;  // 小数点后至少跟一个整数
		}
		for (p++; ISDIGIT(*p); p++);   //跳完小数点后所有整数
	}
	if (*p == 'e' || *p == 'E') {    // 检查指数部分
		p++;
		if (*p == '+' || *p == '-') {  // 指数后面可以有正负号
			p++;
		}
		if (!ISDIGIT(*p))return LEPT_PARSE_INVALID_VALUE;  // e后面要跟任意一个整数
		for (p++; ISDIGIT(*p); p++);  // 跳过整数
	}
	errno = 0;
	v->n = strtod(c->json, NULL); 
	// strtod很强大，合法的不合法的都可以识别出数字，不合法的json字符串已经剔除，仍可以使用strtod识别数字
	// errno，ERANGE是固定套路
	// if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
	// if ((v->n == HUGE_VAL || v->n == -HUGE_VAL))
	// if (errno == ERANGE)不行，TEST_NUMBER(0.0, "1e-10000");超出范围，但仍是浮点数
	if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)) {
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	v->type = LEPT_NUMBER;
	c->json = p;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
		case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
		default: return lept_parse_number(c, v);  // 默认解析数字
		case '\0': return LEPT_PARSE_EXPECT_VALUE;   
	}
}

int lept_parse(lept_value* v, const char* json) {
	lept_context c;
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	int ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			v->type = LEPT_NULL; 
			//"true n"这种也是null类型（之前未处理，认作true类型）
			// 对第一节的错误进行了修正
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

// 访问解析结果的数值，只适用于LEPT_NUMBER
double lept_get_number(const lept_value* v) { 
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->n;
}