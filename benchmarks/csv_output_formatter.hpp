#pragma once

#include "output_formatter.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <fstream>
#include <string>

struct csv_output_formatter : output_formatter
{
  csv_output_formatter(const std::string& filename)
  {
    file.open(filename, std::fstream::in | std::fstream::out | std::fstream::app);
    if (!file.is_open())
      throw std::runtime_error("Failed to open file " + filename);

    std::string line;
    if (std::getline(file, line))
      parse_header(line);

    file.clear();
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.seekp(0, std::ios_base::end);
  }

  virtual void write(const data_record& record) override
  {
    if (header.empty())
      write_header(record);
    else
      validate_record(record);
    write_record(record);
  }

private:
  void parse_header(const std::string& line)
  {
    boost::split(header, line, boost::is_any_of("\t"));
  }

  void write_header(const data_record& record)
  {
    for (size_t i = 0; i < record.entries.size(); ++i)
    {
      auto& entry = record.entries[i];
      header.push_back(entry.first);
      file << entry.first;
      if (i < record.entries.size() - 1) 
        file << '\t';
    }
    file << std::endl;
  }

  void validate_record(const data_record& record)
  {
    if (header.size() != record.entries.size())
      throw std::runtime_error("Size of record does not match that of other records in this CSV file.");

    for (size_t i = 0; i < record.entries.size(); ++i)
      if (header[i] != record.entries[i].first)
        throw std::runtime_error("Header at position " + std::to_string(i) + " are different - expected: " +
          header[i] + "; actual: " + record.entries[i].first);
  }

  void write_record(const data_record& record)
  {
    for (size_t i = 0; i < record.entries.size(); ++i)
    {
      file << record.entries[i].second;
      if (i < record.entries.size() - 1) 
        file << '\t';
    }
    file << std::endl;
  }

  std::fstream file;
  std::vector<std::string> header;
};