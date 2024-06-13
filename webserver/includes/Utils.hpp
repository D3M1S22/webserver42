#pragma once

#include <sstream>

class Utils {
    private:
        Utils();
    public:
        static std::string to_string(int number){
            std::stringstream ss;
            ss << number;
            return ss.str();
        }
        static std::string to_string(char *buff){
            std::stringstream ss;
            ss << buff;
            return ss.str();
        }
        static int to_int(std::string s)
        {
            int pos;
            std::stringstream ss(s);
            ss >> pos;
            if(ss.fail())
                return -1;
            return pos;
        }
        static std::string    replacestring(std::string line, std::string rep_word, std::string rep_by)
        {
            size_t pos = 0;
            while ((pos = line.find(rep_word, pos)) != std::string::npos)
            {
                line.erase(pos, rep_word.length());
                line.insert(pos, rep_by);
                pos += rep_by.length();
            }
            return(line);
        }

};