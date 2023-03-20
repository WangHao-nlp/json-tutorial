#include "leptjson.h"
#include <assert.h>  // assert()
#include <stdlib.h>  // NULL

// 如果第一个字符是自己想要的，则指针移动一步
#define EXPECT(c, ch) do{assert(*c->json==(ch));c->json++;}while(0)

// json语句，字符串
typedef struct {
	const char* json;
}lept_context;


// 解析空白（空格 制表 换行 回车），遇到空格、制表、换行、回车，指针后移一步
static void lept_parse_whitespace(lept_context* c) {   
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	c->json = p; 
}

// 解析LEPT_NULL类型
static int lept_parse_null(lept_context* c, lept_value* v) {  
	EXPECT(c, 'n');
	if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l') {
		return LEPT_PARSE_INVALID_VALUE; // 如果不是null，则是无效值
	}
	c->json += 3;          // 是null;
	v->type = LEPT_NULL;
	return LEPT_PARSE_OK;
}

// 解析LEPT_TRUE类型
static int lept_parse_true(lept_context* c, lept_value* v) {  
	EXPECT(c, 't');
	if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e') {
		return LEPT_PARSE_INVALID_VALUE; // 如果不是true，则是无效值
	}
	c->json += 3; // 是true;
	v->type = LEPT_TRUE;
	return LEPT_PARSE_OK;
}

// 解析LEPT_FALSE类型
static int lept_parse_false(lept_context* c, lept_value* v) {  
	EXPECT(c, 'f');
	if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e') {
		return LEPT_PARSE_INVALID_VALUE; // 如果不是false，则是无效值
	}
	c->json += 4; // 是false;
	v->type = LEPT_FALSE;
	return LEPT_PARSE_OK;
}

// 遇到 n 尝试解析LEPT_NULL类型
// t ➔ true
// f ➔ false
// " ➔ string
// 0 - 9 / -➔ number
// [➔ array
// { ➔ object
static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
		case 'n': return lept_parse_null(c, v);
		case 't': return lept_parse_true(c, v);
		case 'f': return lept_parse_false(c, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;   // 文件结尾
		default: return LEPT_PARSE_INVALID_VALUE;    // 目前还没加入字符串，浮点数，数组，对象；如果不是第一个字符上述。则是无效值
	}
}

int lept_parse(lept_value* v, const char* json) {
	lept_context c;      // 理解为json字符串
	assert(v != NULL);  // v不能为空,必须指向一个lept_value（可以理解为json对象）
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	int ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
			// v->type = LEPT_NULL;  
			// 字符串先去除空白，再检查是不是true、false、null，如果是的话，再去除空白，再检查后面有没有字符
			// 如果有，则解析失败，错误类型为LEPT_PARSE_ROOT_NOT_SINGULAR，如"null n"
		}
	}
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}
