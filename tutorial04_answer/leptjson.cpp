﻿#include "leptjson.h"
#include <assert.h>  // assert()
#include <errno.h> // errno, ERANGE
#include <math.h>  // HUGE_VAL
#include <stdlib.h>  // NULL strtod() malloc(), realloc(), free()
#include <string.h> //memcpy()
#include <stdio.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch) do{assert(*c->json==(ch));c->json++;}while(0)
#define ISDIGIT(ch) ((ch)>='0'&&(ch)<='9')
#define ISDIGIT1TO9(ch) ((ch)>='1'&&(ch)<='9')
#define PUTC(c,ch) do{*(char*)lept_context_push(c,sizeof(char))=(ch);}while(0)

typedef struct {
	const char* json;
	char* stack;
	size_t size, top;
}lept_context;

static void* lept_context_push(lept_context* c, size_t size) {
	void* ret;
	assert(size > 0);
	if (c->top + size >= c->size) {
		if (c->size == 0) {
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		}
		while (c->top + size >= c->size) {
			c->size += c->size >> 1;  
		}
		c->stack = (char*)realloc(c->stack, c->size);
	}
	ret = c->stack + c->top;  
	c->top += size;
	return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}


static void lept_parse_whitespace(lept_context* c) {  
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	c->json = p; 
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 0; literal[i + 1]; i++) {
		if (c->json[i] != literal[i + 1]) {
			return LEPT_PARSE_INVALID_VALUE;
		}
	}
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	const char* p = c->json;
	if (*p == '-')p++;
	if (*p == '0') {
		p++;
	}
	else {
		if (!ISDIGIT1TO9(*p)) {
			return LEPT_PARSE_INVALID_VALUE;  
		}
		for (p++; ISDIGIT(*p); p++);   
	}
	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) {
			return LEPT_PARSE_INVALID_VALUE;  
		}
		for (p++; ISDIGIT(*p); p++);   
	}
	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '+' || *p == '-') {
			p++;
		}
		if (!ISDIGIT(*p))return LEPT_PARSE_INVALID_VALUE;  
		for (p++; ISDIGIT(*p); p++);
	}
	errno = 0;
	v->u.n = strtod(c->json, NULL);
	if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)) {
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	v->type = LEPT_NUMBER;
	c->json = p;
	return LEPT_PARSE_OK;
}

//遇到\u, 解析4位十六进制整数为码点
static const char* lept_parse_hex4(const char* p, unsigned* u) {
	int i;
	*u = 0;
	for (i = 0; i < 4; i++) {
		char ch = *p++;
		*u <<= 4;// 乘16, 二进制左移四位
		if (ch >= '0' && ch <= '9')  *u |= ch - '0';
		else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
		else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
		else return NULL;
	}
	//printf("------%s\n", u);
	return p;
}

// 把码点编码成utf-8
static void lept_encode_utf8(lept_context* c, unsigned u) {
	if (u <= 0x7F)
		PUTC(c, u & 0xFF);
	else if (u <= 0x7FF) {
		// 110xxxxx 10xxxxxx
		// 0xC0 1100 0000
		// 300 100101100 12c
		// 100101100>>6 = 100
		// 100&0xFF = 0000 0100
		// 0xC0|(0000 0100) = 1100 0100
		// 100101100 & 0011 1111 = 010 1100
		// 
		PUTC(c, 0xC0 | ((u >> 6) & 0xFF)); // 取后六位之前的部分，然后补到1100 0000里面
		PUTC(c, 0x80 | (u & 0x3F)); // 取后六位，然后补到1000 0000里面
	}
	else if (u <= 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0xFF)); // 取后十二位之前的部分，然后补到1110 0000里面
		PUTC(c, 0x80 | ((u >> 6) & 0x3F)); // 取后6位到12之间的部分，然后补到1000 0000里面
		PUTC(c, 0x80 | (u & 0x3F)); // 取后六位，然后补到1000 0000里面
	}
	else {
		assert(u <= 0x10FFFF);
		PUTC(c, 0xF0 | ((u >> 18) & 0xFF)); //取后18位之前的部分，然后补到1111 0000里面
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)
// #define STRING_ERROR(ret) do { return ret; } while(0)

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	unsigned u, u2;  
	const char* p;
	EXPECT(c, '\"');
	p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			len = c->top - head;
			lept_set_string(v, (const char*)lept_context_pop(c, len), len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\\':
			switch (*p++) {
				case '\"': PUTC(c, '\"'); break;
				case '\\': PUTC(c, '\\'); break;
				case '/':  PUTC(c, '/'); break;
				case 'b':  PUTC(c, '\b'); break;
				case 'f':  PUTC(c, '\f'); break;
				case 'n':  PUTC(c, '\n'); break;
				case 'r':  PUTC(c, '\r'); break;
				case 't':  PUTC(c, '\t'); break;
				case 'u':
					if (!(p = lept_parse_hex4(p, &u))) // 解析失败，无效四位十六进制码点
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
					// 代理对 "\"\\uD834\\uDD1E\""
					if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
						if (*p++ != '\\') // 后面没跟低代理项
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						if (*p++ != 'u') // 后面没跟低代理项
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						if (!(p = lept_parse_hex4(p, &u2))) // 低代理项解析失败，无效四位十六进制码点
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
						if (u2 < 0xDC00 || u2 > 0xDFFF)  // 低代理项解析结果不符合规定
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						// 合并代理对 codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
						u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
					}
					lept_encode_utf8(c, u);
					break;
				default:
					STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
			}
			break;
		case '\0':
			STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
		default:
			if ((unsigned char)ch < 0x20) {
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
			}
			PUTC(c, ch);
		}
	}
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
		case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
		default: return lept_parse_number(c, v);
		case '"':return lept_parse_string(c, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;   // 文件结尾
	}
}

int lept_parse(lept_value* v, const char* json) {
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	c.stack = NULL;
	c.size = c.top = 0;
	lept_init(v);
	lept_parse_whitespace(&c);
	ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			v->type = LEPT_NULL; //"true n"这种也是null类型（之前未处理，认作true类型）
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	// 字符串先去除空白，再检查是不是true、false、null，如果是的话，再去除空白，再检查后面有没有字符
	assert(c.top==0);
	free(c.stack);
	return ret;
}

void lept_free(lept_value* v) {
	assert(v != NULL);
	if (v->type == LEPT_STRING) {
		free(v->u.s.s);
	}
	v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

int lept_get_boolean(const lept_value* v) {
	assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
	return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
	lept_free(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
	lept_free(v);
	v->u.n = n;
	v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);
	v->u.s.s[len] = '\0';
	v->u.s.len = len;
	v->type = LEPT_STRING;
}