#pragma once

#include "output_formatter.hpp"

#include <iostream>

struct console_output_formatter : output_formatter
{
  virtual void write(const data_record& record) override
  {
    if (first_output)
    {
      write(record, false);
      std::cout << "====================" << std::endl;
      first_output = false;
    }
    write(record, true);
  }
private:
  void write(const data_record& record, bool ops_only)
  {
    for (auto& entry : record.entries)
      if (entry.first != "ns/op" ^ ops_only)
        std::cout << entry.first << ": " << entry.second << std::endl;
  }

  bool first_output = true;
};