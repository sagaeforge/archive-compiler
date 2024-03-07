//
// Created by nugdev on 3/6/24.
//

#include "File.h"

#include <fstream>
#include <sstream>
#include <regex>

#include "Comment.h"
#include "Section.h"
#include "Field.h"

namespace nugdev::ndk::ini {

void File::parse() {
  std::wifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("File not found");
  }

  const auto is_space_line = [](const std::wstring& _line) -> bool {
    return std::regex_match(_line, std::wregex(L"\\s*"));
  };

  const auto is_comment_line = [](const std::wstring& _line) -> bool {
    return std::regex_match(_line, std::wregex(L"\\s*;.*"));
  };

  const auto is_section_line = [](const std::wstring& _line) -> bool {
    return std::regex_match(_line, std::wregex(L"\\s*\\[.*\\].*"));
  };

  std::wstring section;
  std::wstring line;
  for (std::uint32_t lineCnt = 1; std::getline(file, line); ++lineCnt) {
    if (is_space_line(line)) {
      lines.push_back(Line{lineCnt, std::make_shared<Content>()});
      continue;
    }

    if (is_comment_line(line)) {
      std::wsmatch match;
      std::regex_search(line, match, std::wregex(L"\\s*;\\s*(.*)"));
      lines.push_back(Line{lineCnt, std::make_shared<Comment>(match[1])});
      continue;
    }

    if (is_section_line(line)) {
      std::wsmatch match;
      std::regex_search(line, match, std::wregex(L"\\s*\\[(.*)\\](.*)"));
      section = match[1].str();

      lines.push_back(Line{lineCnt, std::make_shared<Section>(section, std::make_shared<Comment>(match[2].str()))});
      continue;
    }

    std::wstring key;
    std::wstring value;
    std::wstring comment;
    if (line.find(L';') != std::wstring::npos) {
      std::wsmatch match;
      std::regex_search(line, match, std::wregex(L"\\s*(.*)\\s*=\\s*(.*)\\s*;\\s*(.*)"));

      key = match[1].str();
      value = match[2].str();
      comment = match[3].str();

      // value의 rtrim 해줌
      std::wstringstream ss(value);
      ss >> value;

    } else {
      std::wsmatch match;
      std::regex_search(line, match, std::wregex(L"\\s*(.*)\\s*=\\s*(.*)"));

      key = match[1].str();
      value = match[2].str();
    }

    // value 앞뒤로 공백 제거 및 따옴표 제거
    value = std::regex_replace(value, std::wregex(L"\\s*\"(.*)\"\\s*"), L"$1");

    lines.push_back(Line{lineCnt, std::make_shared<Field>(section, key, value, std::make_shared<Comment>(comment))});
  }

  file.close();
}

void File::save() {
  this->save(this->path);
}

void File::save(const std::wstring& _path) {
  std::wofstream file(_path);
  if (!file.is_open()) {
    throw std::runtime_error("File not found");
  }

  for (auto itr = 0; itr < this->lines.size(); ++itr) {
    const auto& [number, content] = this->lines[itr];

    switch (content->getType()) {
      case ContentType::None: break;
      case ContentType::Comment: {
        const auto comment = std::dynamic_pointer_cast<Comment>(content);
        file << L"; " << comment->comment;
      }
      break;
      case ContentType::Section: {
        const auto section = std::dynamic_pointer_cast<Section>(content);
        file << L"[" << section->name << L"] ";

        if (!section->comment->comment.empty()) {
          file << L"; " << section->comment->comment;
        }
      }
      break;
      case ContentType::Field: {
        const auto field = std::dynamic_pointer_cast<Field>(content);
        file << field->key << L" = ";

        if (std::isspace(field->value.front()) || std::isspace(field->value.back())) {
          file << L"\"" << field->value << L"\"";
        } else {
          file << field->value;
        }

        if (!field->comment->comment.empty()) {
          file << L" ; " << field->comment->comment;
        }
      }
      break;
    }

    if (itr + 1 < this->lines.size()) {
      file << std::endl;
    }
  }

  file.close();
}

std::wostream& operator<<(std::wostream& _os, const File& _file) {
  for (const auto& line: _file.lines) {
    auto content = line.content;

    switch (content->getType()) {
      case ContentType::None:
        _os << line.number << L": Empty Line" << std::endl;
      break;
      case ContentType::Comment: {
        const auto comment = std::dynamic_pointer_cast<Comment>(content);
        _os << line.number << L": Comment: [" << comment->comment << "]" << std::endl;
      }
      break;
      case ContentType::Section: {
        const auto section = std::dynamic_pointer_cast<Section>(content);
        _os << line.number << L": Section: [" << section->name << L"] Comment: " << section->comment->comment << L"]" << std::endl;
      }
      break;
      case ContentType::Field: {
        const auto field = std::dynamic_pointer_cast<Field>(content);
        _os << line.number << L": Section: [" << field->section << L"] Key: [" << field->key << L"] Value: [" << field->value << L"] Comment: [" << field->comment->comment << L"]" << std::endl;
      }
      break;
    }
  }

  return _os;
}

}