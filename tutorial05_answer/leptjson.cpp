#include "leptjson.h"
#include <assert.h>  // assert()
#include <errno.h> // errno, ERANGE
#include <math.h>  // HUGE_VAL
#include <stdlib.h>  // NULL strtod() malloc(), realloc(), free()
#include <string.h> //memcpy()
#include<stdio.h>

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

static const char* lept_parse_hex4(const char* p, unsigned* u) {
	int i;
	*u = 0;
	for (i = 0; i < 4; i++) {
		char ch = *p++;
		*u <<= 4;
		if (ch >= '0' && ch <= '9')  *u |= ch - '0';
		else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
		else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
		else return NULL;
	}
	return p;
}

static void lept_encode_utf8(lept_context* c, unsigned u) {
	if (u <= 0x7F)
		PUTC(c, u & 0xFF);
	else if (u <= 0x7FF) {
		PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
		PUTC(c, 0x80 | (u & 0x3F));
	}
	else if (u <= 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
	else {
		assert(u <= 0x10FFFF);
		PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	unsigned u, u2;  // ?
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
					if (!(p = lept_parse_hex4(p, &u)))
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
					if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
						if (*p++ != '\\')
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						if (*p++ != 'u')
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
						if (!(p = lept_parse_hex4(p, &u2)))
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
						if (u2 < 0xDC00 || u2 > 0xDFFF)
							STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
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
static int lept_parse_value(lept_context* c, lept_value* v);

static int lept_parse_array(lept_context* c, lept_value* v) {
	size_t i, size = 0;
	int ret;
	EXPECT(c, '[');  // 识别到方括号
	lept_parse_whitespace(c);  // 去除空值符号
	if (*c->json == ']') {    // 如果识别到右方括号，直接解析结束，数组指针指向空
		c->json++;
		v->type = LEPT_ARRAY;
		v->u.a.size = 0;
		v->u.a.e = NULL;
		return LEPT_PARSE_OK;  
	}
	for (;;) {   // 挨个解析数组元素（可能是任何json对象）
		lept_value e;  // 重新初始化个json对象用来存储解析时候的临时变量
		lept_init(&e);
		if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) {  // 解析元素中的对象失败
			break;
			// 不能直接返回ret，要free后return，防止内存泄露
		}
		//printf("************%d\n", sizeof(lept_value));
		//printf("************%d\n", sizeof(lept_value::u));
		//printf("************%d\n", sizeof(lept_value::u.n));
		//printf("************%d\n", sizeof(lept_value::u.a));
		//printf("************%d\n", sizeof(lept_value::u.s));
		//printf("************%d\n", sizeof(lept_value::u.s.len));
		//printf("************%d\n", sizeof(lept_value::u.s.s));
		//printf("************%d\n\n", sizeof(lept_value::type));

		// 识别成功，则将临时变量存进json语句的stack中
		// 尽管每个json对象的种类可能不同，但由于按最大需要的内存进行存储，所以不会发生冲突
		// 比如数字只需要8字节（变量+类型）；但字符串和数组都需要16字节（指针+大小+类型）
		// 但存储变量也用16字节，所以不需要考虑存储不同json类型时候的问题
		// 都是直接存16个字节的指针
		memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));  // 任意类型均可
		size++;
		lept_parse_whitespace(c);
		if (*c->json == ',') {   // 遇到逗号, 继续解析
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*c->json==']') {  // 解析结束
			c->json++;
			v->type = LEPT_ARRAY;
			v->u.a.size = size;
			size *= sizeof(lept_value); 
			// 解析结束后，将json语句中的stack存进json对象中数组的指针里
			memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
			//printf("%d\n", lept_get_type(lept_get_array_element(v, 1)));
			return LEPT_PARSE_OK;
		}
		else {
			// 解析完一个元素后，缺少逗号和中括号
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}
	for (i = 0; i < size; i++)
		// 从后往前依次释放json值内存
		lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
	return ret;
}


static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
		case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
		default: return lept_parse_number(c, v);
		case '"':return lept_parse_string(c, v);
		case '[':return lept_parse_array(c, v);
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
			v->type = LEPT_NULL; 
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(c.top==0);
	free(c.stack);
	return ret;
}

void lept_free(lept_value* v) {
	size_t i;
	assert(v != NULL);
	switch (v->type) {
	case LEPT_STRING:
		free(v->u.s.s);
		break;
	case LEPT_ARRAY:
		for (i = 0; i < v->u.a.size; i++)
			lept_free(&v->u.a.e[i]);
		free(v->u.a.e);
		break;
	default: break;
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

size_t lept_get_array_size(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value*v,size_t index) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	assert(index < v->u.a.size);
	return &v->u.a.e[index];
}

