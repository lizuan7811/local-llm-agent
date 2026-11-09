#ifndef LIYONG_OLLAMA_AGENT_GENERICXMLPARSER_H
#define LIYONG_OLLAMA_AGENT_GENERICXMLPARSER_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class XmlNode
{
private:
    std::string name_;
    std::string text_;
    std::unordered_map<std::string, std::string> attributes_;
    std::vector<XmlNode> childrens_;

public:
    std::string getName() const { return name_; }
    std::string getText() const { return text_; }

    const std::unordered_map<std::string, std::string>& getAttributes() const { return attributes_; }
    std::unordered_map<std::string, std::string>& getAttributes() { return attributes_; }

    const std::vector<XmlNode>& getChildrens() const { return childrens_; }

    std::string getChildText(const std::string& childName) const
    {
        for (const auto& child : childrens_)
        {
            if (child.getName() == childName) return child.getText();
        }
        return "";
    }

    void setName(const std::string& name) { name_ = name; }
    void setText(const std::string& text) { text_ = text; }
    void setAttributes(const std::unordered_map<std::string, std::string>& attributes) { attributes_ = attributes; }
    void setChildrens(const std::vector<XmlNode>& childrens) { childrens_ = childrens; }
    void addChildren(const XmlNode& children) { childrens_.push_back(children); }
};

// 重點：呼叫端必須指定「原始文字標籤」(rawTags)，例如 {"content"}。
// 這類標籤的內容不會被當成 XML 遞迴解析，所以裡面的 C++ (#include <iostream>、template<...>) 不會被誤判。
// 原始文字標籤支援 <![CDATA[ ... ]]>，也支援 &lt; 等實體還原，或直接寫原始字元。
class GenericXmlParser
{
public:
    using RawTags = std::unordered_set<std::string>;

private:
    static void skipWhiteSpace(const std::string& xml, size_t& pos)
    {
        while (pos < xml.size() && std::isspace(static_cast<unsigned char>(xml[pos]))) ++pos;
    }

    // 單次掃描的實體還原；不會把 &amp;lt; 還原兩次
    static std::string decodeEntities(const std::string& s)
    {
        static const std::pair<const char*, char> table[] = {
            {"&lt;", '<'}, {"&gt;", '>'}, {"&amp;", '&'}, {"&quot;", '"'}, {"&apos;", '\''}
        };
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size();)
        {
            bool hit = false;
            if (s[i] == '&')
            {
                for (const auto& [ent, ch] : table)
                {
                    size_t n = std::char_traits<char>::length(ent);
                    if (s.compare(i, n, ent) == 0)
                    {
                        out += ch;
                        i += n;
                        hit = true;
                        break;
                    }
                }
            }
            if (!hit) out += s[i++];
        }
        return out;
    }

    static constexpr const char* kCdataOpen = "<![CDATA[";

public:
    // 解析成功時 pos 指到節點結尾之後；失敗回傳 nullopt（pos 不保證，呼叫端自行重設）
    static std::optional<XmlNode> parseNode(const std::string& xml, size_t& pos, const RawTags& rawTags = {})
    {
        skipWhiteSpace(xml, pos);
        if (pos >= xml.size() || xml[pos] != '<') return std::nullopt;
        if (xml.compare(pos, 4, "<!--") == 0) return std::nullopt;
        if (xml.compare(pos, 9, kCdataOpen) == 0) return std::nullopt;

        ++pos; // 跳過 '<'

        size_t nameStart = pos;
        while (pos < xml.size() &&
               !std::isspace(static_cast<unsigned char>(xml[pos])) &&
               xml[pos] != '>' && xml[pos] != '/')
        {
            ++pos;
        }

        XmlNode node;
        node.setName(xml.substr(nameStart, pos - nameStart));
        if (node.getName().empty() || node.getName()[0] == '/') return std::nullopt;

        // 解析屬性
        while (true)
        {
            skipWhiteSpace(xml, pos);
            if (pos >= xml.size()) return std::nullopt;
            if (xml[pos] == '>' || xml[pos] == '/') break;

            size_t attrNameStart = pos;
            while (pos < xml.size() && xml[pos] != '=' && xml[pos] != '>' && xml[pos] != '/' &&
                   !std::isspace(static_cast<unsigned char>(xml[pos])))
            {
                ++pos;
            }
            std::string attrName = xml.substr(attrNameStart, pos - attrNameStart);
            skipWhiteSpace(xml, pos);

            if (pos < xml.size() && xml[pos] == '=')
            {
                ++pos;
                skipWhiteSpace(xml, pos);
                if (pos < xml.size() && (xml[pos] == '"' || xml[pos] == '\''))
                {
                    char quote = xml[pos++];
                    size_t valStart = pos;
                    while (pos < xml.size() && xml[pos] != quote) ++pos;
                    if (pos >= xml.size()) return std::nullopt;
                    node.getAttributes()[attrName] = decodeEntities(xml.substr(valStart, pos - valStart));
                    ++pos; // 結尾引號
                }
            }
            else if (attrName.empty())
            {
                ++pos; // 避免無法前進造成無窮迴圈
            }
        }

        // 自關閉標籤 <br/>
        if (xml[pos] == '/')
        {
            size_t gt = xml.find('>', pos);
            if (gt == std::string::npos) return std::nullopt;
            pos = gt + 1;
            return node;
        }
        ++pos; // 跳過 '>'

        const std::string closeTag = "</" + node.getName() + ">";

        // ---- 原始文字標籤：不遞迴 ----
        if (rawTags.count(node.getName()))
        {
            size_t p = pos;
            skipWhiteSpace(xml, p);
            if (xml.compare(p, 9, kCdataOpen) == 0)
            {
                size_t st = p + 9;
                size_t en = xml.find("]]>", st);
                if (en == std::string::npos) return std::nullopt;
                std::string text = xml.substr(st, en - st);
                if (!text.empty() && text.front() == '\n') text.erase(0, 1);
                size_t after = en + 3;
                skipWhiteSpace(xml, after);
                if (xml.compare(after, closeTag.size(), closeTag) != 0) return std::nullopt;
                node.setText(text);
                pos = after + closeTag.size();
                return node;
            }

            size_t close = xml.find(closeTag, pos);
            if (close == std::string::npos) return std::nullopt;
            std::string text = xml.substr(pos, close - pos);
            if (!text.empty() && text.front() == '\n') text.erase(0, 1);
            node.setText(decodeEntities(text));
            pos = close + closeTag.size();
            return node;
        }

        // ---- 一般標籤：文字 + 子標籤 ----
        std::string text;
        while (pos < xml.size())
        {
            size_t nextTag = xml.find('<', pos);
            if (nextTag == std::string::npos) return std::nullopt; // 沒有結尾標籤

            text.append(xml, pos, nextTag - pos);

            if (xml.compare(nextTag, closeTag.size(), closeTag) == 0)
            {
                if (!text.empty() && text.front() == '\n') text.erase(0, 1);
                node.setText(decodeEntities(text));
                pos = nextTag + closeTag.size();
                return node;
            }

            if (xml.compare(nextTag, 9, kCdataOpen) == 0)
            {
                size_t en = xml.find("]]>", nextTag + 9);
                if (en == std::string::npos) return std::nullopt;
                text.append(xml, nextTag + 9, en - (nextTag + 9)); // CDATA 內容不做實體還原
                pos = en + 3;
                continue;
            }

            if (xml.compare(nextTag, 4, "<!--") == 0)
            {
                size_t en = xml.find("-->", nextTag + 4);
                if (en == std::string::npos) return std::nullopt;
                pos = en + 3;
                continue;
            }

            pos = nextTag;
            auto child = parseNode(xml, pos, rawTags);
            if (!child.has_value()) return std::nullopt; // 子標籤壞掉：整個節點視為失敗，不吞掉後面內容
            node.addChildren(child.value());
        }
        return node;
    }

    // 批量解析所有 <targetTag> 根節點
    static std::vector<XmlNode> parseAllRootNodes(const std::string& rawXml,
                                                  const std::string& targetTag,
                                                  const RawTags& rawTags = {})
    {
        std::vector<XmlNode> results;
        size_t pos = 0;
        const std::string openTag = "<" + targetTag;

        while ((pos = rawXml.find(openTag, pos)) != std::string::npos)
        {
            size_t start = pos;
            auto parsed = parseNode(rawXml, pos, rawTags);
            if (parsed.has_value())
            {
                results.push_back(std::move(parsed.value()));
            }
            else
            {
                pos = start + 1; // 失敗就從下一個字元重找，不跳過後面的區塊
            }
        }
        return results;
    }
};

#endif //LIYONG_OLLAMA_AGENT_GENERICXMLPARSER_H