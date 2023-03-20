#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum {
	LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LRPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT
} lept_type;
//      json的数据类型：包括空，布尔，浮点数，字符串，数组，对象
//null: 表示为 null
//boolean : 表示为 true 或 false
//number : 浮点数
//string : 表示为 "abc"
//array : 表示为[null,true,number,string,[,,]]
//object : 表示为{"k1":null,"k2":true,"k3":123,"k4":"value4","k5":[1,2],"k6":{}}

typedef struct {
	lept_type type;
}lept_value;
// json值，由于第一章只检测空和布尔，所以里面只有数据类型（空和布尔类型的值即为他们的类型）
// 后续会加入字符串的内容，浮点数的数值，数组的成员、大小、容量，对象的成员、大小、容量，


// 解析器解析结果，前几章都是一个个检测，即一次检测只能检测一种类型，不能检测复合的json对象（第六章介绍）
// 一开始只能检测"123"是数字，"true"是布尔，"\"abc\""是字符串
enum {
	LEPT_PARSE_OK = 0,                // 无错误
	LEPT_PARSE_EXPECT_VALUE,          // 空, 如 " ",""
	LEPT_PARSE_INVALID_VALUE,         // 无效值，检测结果不是null，true，false，
	LEPT_PARSE_ROOT_NOT_SINGULAR      // 结果后面还有其他值，"null n"
};

int lept_parse(lept_value* v, const char* json);  // 解析函数，传入字符串，解析出json对象

lept_type lept_get_type(const lept_value* v);     
// 访问解析结果，这个只访问对象类型：如"null"为LEPT_NULL，"123"为LRPT_NUMBER

#endif // !LEPTJSON_H__