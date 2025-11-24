
#include <sstream>

template<typename T>
std::string format_number(T number) {
    std::string str = std::to_string(number);
    std::stringstream ss;
    for (int i = static_cast<int>(str.length() - 1); i >= 0;) {
        ss << str[i];//inserting 1st digit
        --i;
        if (i == -1)break;//checking if we are out of left bound
        ss << str[i];//inserting 2nd digit
        --i;
        if (i == -1)break;//checking if we are out of left bound
        ss << str[i];//inserting 3rd digit
        --i;
        if (i == -1)break;//checking if we are out of left bound
        ss << " ";//after 3 digits insertion, finally inserting a dot "."
    }
    str = ss.str();
    reverse(str.begin(), str.end()); //reversing the final string
    return str;
}

//auto fmt::formatter<unsigned>::format(const unsigned value) {
//    return format_number(value);
//}
//
//
//auto fmt::formatter<unsigned long>::format(const unsigned long value) {
//    return format_number(value);
//}
//
//auto fmt::formatter<unsigned long long>::format(const size_t value) {
//    return format_number(value);
//}
