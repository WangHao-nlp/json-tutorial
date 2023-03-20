#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum {
	LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT
} lept_type;

typedef struct {
	double n;
	lept_type type;
}lept_value;

enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG   // 解析失败，数值过大
};

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

double lept_get_number(const lept_value* v); // 访问解析结果的数值，只适用于LEPT_NUMBER

#endif // !LEPTJSON_H__
