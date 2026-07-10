#include "log_crypto.h"

static const char* HEX = "0123456789abcdef";//16进制字符表0-15

std::string xor_encrypt_to_hex(const std::string& plain, unsigned char key) //明文XOR加密并转成十六进制字符串
{
    std::string out;
    out.reserve(plain.size() * 2);//每个字节转化为2个十六进制字符，长度乘以2
    for (unsigned char ch : plain) //遍历每个字符
    {
        unsigned char e = static_cast<unsigned char>(ch ^ key);//对字符用key做XOR加密
        out.push_back(HEX[(e >> 4) & 0xF]);//取出高四位转化为十六进制，作为HEX数组索引
        out.push_back(HEX[e & 0xF]);//取出低四位转化为十六进制作为HEX数组索引
    }
    return out;
}

int hex_char_to_value(char c) //单个十六进制字符转为数值
{
    if (c >= '0' && c <= '9') return c - '0';//字符减去'0'得到数值
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;//字符减去'a'加十得到数值
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;//字符减去'A'加十得到数值
    return -1;//非法字符
}

bool is_valid_hex_string(const std::string& s) //验证十六进制字符串是否合法
{
    if (s.empty() || (s.size() % 2) != 0) return false;
    for (char c : s)
        if (hex_char_to_value(c) < 0) return false;
    return true;
}

std::string xor_decrypt_from_hex(const std::string& hex, unsigned char key) 从十六进制字符串解密还原明文
{
    if (!is_valid_hex_string(hex)) return {};//先验证十六进制字符串是否合法，不合法返回空串
    std::string out;
    out.reserve(hex.size() / 2);//两个字符对应一个字节，长度除以二
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) //每次处理两个字符i和i+1，i每次加2
    {
        int hi = hex_char_to_value(hex[i]);//第一个字符对应高四位
        int lo = hex_char_to_value(hex[i + 1]);//第二个字符对应低四位
        unsigned char e = static_cast<unsigned char>((hi << 4) | lo);//合成一个完整字节
        out.push_back(static_cast<char>(e ^ key));//还原明文
    }
    return out;
}
