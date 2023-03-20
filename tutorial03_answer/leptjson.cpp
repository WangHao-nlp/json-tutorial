#include "leptjson.h"
#include <assert.h>    // assert()
#include <errno.h>     // errno, ERANGE
#include <math.h>      // HUGE_VAL
#include <stdlib.h>    // NULL strtod() malloc(), realloc(), free()
#include <string.h>    //memcpy()
#include <stdio.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif
// 栈初始化长度

#define EXPECT(c, ch) do{assert(*c->json==(ch));c->json++;}while(0)
#define ISDIGIT(ch) ((ch)>='0'&&(ch)<='9')
#define ISDIGIT1TO9(ch) ((ch)>='1'&&(ch)<='9')
#define PUTC(c,ch) do{ *(char*)lept_context_push(c,sizeof(char)) = ch;}while(0)
// 这一步赋值

typedef struct {
	const char* json;
	char* stack;
	size_t size, top;
}lept_context;
// json语句，在char* json后，加入stack，size，top，用来解析包含转义字符的json
// 我们解析字符串（以及之后的数组、对象）时，需要把解析的结果先储存在一个临时的缓冲区，最后再用lept_set_string() 把缓冲区的结果设进值之中。
// 在完成解析一个字符串之前，这个缓冲区的大小是不能预知的。
// 因此，我们可以采用动态数组（dynamic array）这种数据结构，即数组空间不足时，能自动扩展。

// 往json栈中加字符
static void* lept_context_push(lept_context* c, size_t size) {
	void* ret;
	assert(size > 0);
	if (c->top + size >= c->size) {
		if (c->size == 0) {
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		}
		while (c->top + size >= c->size) {
			c->size += c->size >> 1;  // c.size*1.5
		}
		c->stack = (char*)realloc(c->stack, c->size);
		// a） 如果可能，扩大或缩小ptr所指的现有区域。该区域的内容保持不变，直到新尺寸和旧尺寸中较小的尺寸。
		// 如果区域被扩展，则数组新部分的内容未定义。
		// b） 分配大小为newsize字节的新内存块，复制大小等于新大小和旧大小中较小值的内存区域，并释放旧块。
		// 如果内存不足，则不会释放旧内存块，并返回空指针。
	}
	ret = c->stack + c->top;  // 目前指向的字符串位置
	c->top += size;
	//printf("%s\n", c->stack);
	return ret;
}

// 从json栈中取字符
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

// 我们只需要先备份栈顶，然后把解析到的字符压栈，最后计算出长度并一次性把所有字符弹出，再设置至值里便可以。

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	const char* p;
	EXPECT(c, '\"');  // 字符串类型，以\"开头
	p = c->json;
	for (;;) {
		char ch = *p++;  // 获取一个字符
		switch (ch) {
			case '\"':   // 如果是\", 解析结束
				len = c->top - head;
				// printf("*****%d\n", head);  // head始终是0，不懂head的意义，也不懂为啥每次要将top置为0
				lept_set_string(v, (const char*)lept_context_pop(c, len), len); // 给json对象中的string部分赋值
				//printf("****%s\n", v->u.s.s);
				c->json = p; // json字符串只保留剩余未检测部分
				return LEPT_PARSE_OK;
			case '\\':    // 遇到转义字符，c语言中字符串的\\,表示一个转义字符\ 
				switch (*p++) {   // 遇到转义字符就向栈中添加一个字符
					case '\"': PUTC(c, '\"'); break;
					case '\\': PUTC(c, '\\'); break;
					case '/':  PUTC(c, '/'); break;
					case 'b':  PUTC(c, '\b'); break;
					case 'f':  PUTC(c, '\f'); break;
					case 'n':  PUTC(c, '\n'); break;
					case 'r':  PUTC(c, '\r'); break;
					case 't':  PUTC(c, '\t'); break;
					default:  // 如果不是上述符号，则遇到了无效的转义字符
						c->top = head;  // 这句话似乎无用
						return LEPT_PARSE_INVALID_STRING_ESCAPE;
				}
				break;
			case '\0':   // 遇到了字符串结束负号，则意味着c语言中的字符串结束，json字符串缺少引号
				//printf("c->top:%d\n", c->top);
				//printf("head:%d\n", head);
				c->top = head;  
				// "\"abc" , 解析失败，必须栈顶指针置为0 
				return LEPT_PARSE_MISS_QUOTATION_MARK;
			default:
				if ((unsigned char)ch < 0x20) {  // 遇到了解析不出来的字符
					c->top = head;
					return LEPT_PARSE_INVALID_STRING_CHAR;
				}
				PUTC(c, ch);  // 可以正常解析的字符
		}
	}
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
		case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
		case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
		case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
		default: return lept_parse_number(c, v);
		case '\"':return lept_parse_string(c, v);
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