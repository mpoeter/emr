#pragma once

#include <string>
#include <utility>
#include <vector>

struct data_record
{
  std::vector<std::pair<std::string, std::string>> entries;
  void add(std::string name, std::string value)
  {
    entries.push_back(std::make_pair(std::move(name), std::move(value)));
  }
};

struct output_formatter
{
  virtual ~output_formatter() {}
  virtual void write(const data_record& record) = 0;
};