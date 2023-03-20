#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include<stddef.h> // size_t

typedef enum {
	LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT
} lept_type;


// 变体类型，type 来决定它现时是哪种类型，而这也决定了哪些成员是有效的。
typedef struct {
	union {
		struct { char* s; size_t len; }s;   // string
		double n;                           // number
	}u;
	lept_type type;
}lept_value;


enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,           // 缺少引号
	LEPT_PARSE_INVALID_STRING_ESCAPE,         // 无效字符串转义
	LEPT_PARSE_INVALID_STRING_CHAR            // 无效的字符
};

#define lept_init(v) do{(v)->type=LEPT_NULL;}while(0)

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v);

lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)          // 将json对象指针指向空

int lept_get_boolean(const lept_value* v);   
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

#endif // !LEPTJSON_H__
