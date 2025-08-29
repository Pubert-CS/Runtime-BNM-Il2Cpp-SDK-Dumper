#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

class CppCodeWriter {
private:
    std::stringstream top;
    std::stringstream bottom;
    std::ostream& writer;
    int indent;
    std::string tabString;
    std::string lastLine = "";

    bool ShouldIndent(const std::string& line) {
        return lastLine == "";
    }

public:
    CppCodeWriter(std::ostream& writer) : writer(writer), indent(0), tabString("\t") {}

    CppCodeWriter(std::ostream& writer, const std::string& tabString)
            : writer(writer), indent(0), tabString(tabString) {}

    void WriteLine(const std::string& line = "") {
        if (ShouldIndent(line)) {
            for (int i = 0; i < indent; i++) {
                bottom << tabString;
            }
        }
        bottom << line << std::endl;
        lastLine = "";
    }

    void WriteTopLine(const std::string& line = "") {
        top << line << std::endl;
        lastLine = "";
    }

    void Write(const std::string& line = "") {
        if (ShouldIndent(line)) {
            for (int i = 0; i < indent; i++) {
                bottom << tabString;
            }
        }
        bottom << line;
        lastLine = line;
    }

    void WriteComment(const std::string& comment) {
        WriteLine("// " + comment);
    }

    void WriteInclude(const std::string& include) {
        WriteTopLine("#include <" + include + ">");
    }

    void WriteBracket() {
        WriteLine("{");
        indent++;
    }

    void CloseBracket(const std::string& suffix = "") {
        indent--;
        WriteLine("}" + suffix);
    }

    std::string preview() {
        std::stringstream ss;
        ss << top.str();
        ss << std::endl;
        ss << bottom.str();
        return ss.str();
    }

    void WriteToFile() {
        writer << top.str();
        writer << std::endl;
        writer << bottom.str();
    }
};
