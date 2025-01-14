﻿#include <xlsx_typed_worksheet.h>
#include <iostream>
#include <typed_string/arena_typed_string_parser.h>

namespace {
	using namespace spiritsaway::xlsx_reader;
	using namespace std;

}
namespace spiritsaway::xlsx_reader{
	using namespace std;
	typed_header::typed_header(const typed_string_desc* in_type_desc, string_view in_header_name, string_view in_header_comment):type_desc(in_type_desc), header_name(in_header_name), header_comment(in_header_comment)
	{

	}
	std::ostream& operator<<(std::ostream& output_stream, const typed_header& in_typed_header)
	{
		output_stream<<"name: "<< in_typed_header.header_name<<" type_desc: "<<*in_typed_header.type_desc<<" comment:"<<in_typed_header.header_comment<<endl;
		return output_stream;
	}

	
	bool typed_worksheet::convert_typed_header()
	{
		typed_headers.clear();
		typed_headers.push_back(nullptr);
		int column_idx = 1;
		if (get_max_row() < 1)
		{
			return false;
		}
		const auto& header_name_row = get_row(1);
		if (header_name_row.empty())
		{
			return false;
		}
		for(std::size_t i= 1; i< header_name_row.size(); i++)
		{			
			auto cur_header_name = get_cell(1, i);
			if(cur_header_name.empty())
			{
				cerr<<"empty header name at idx "<<i<<endl;
				return false;
			}

			auto cur_cell_value = get_cell(2, column_idx);

			if (cur_cell_value.empty())
			{
				cerr <<"invalid type desc for header type at column " << i << endl;
			}
			auto cur_type_desc = typed_string_desc::get_type_from_str(&memory_arena, cur_cell_value);
			string_view header_comment = get_cell(3, column_idx);
			typed_headers.push_back(new typed_header(cur_type_desc, cur_header_name, header_comment));

			if (column_idx == 1)
			{
				// expect int or str in first column
				switch (cur_type_desc->m_type)
				{
				case basic_value_type::number_bool:
				case basic_value_type::number_float:
				case basic_value_type::number_double:
				case basic_value_type::comment:
				case basic_value_type::list:
				case basic_value_type::tuple:
					cerr << "first column value type should be int or string" << endl;
					return false;
				default:
					break;
				}
			}
			if (get_header_idx(cur_header_name) != 0)
			{
				cerr << "duplicated header name " << cur_header_name << endl;
			}
			header_column_index[cur_header_name] = column_idx;
			column_idx += 1;
		}
		return true;
	}
	std::uint32_t typed_worksheet::value_begin_row() const
	{
		return 4;
	}
	void typed_worksheet::convert_cell_to_arena_typed_value()
	{
		arena_typed_string_parser cur_parser(memory_arena);
		all_cell_values.clear();
		auto value_begin_row_idx = value_begin_row();
		const auto& all_row_info = get_all_row();
		if (value_begin_row_idx > max_rows)
		{
			all_cell_values.emplace_back();
			return;
		}
		all_cell_values.reserve(1 + max_rows - value_begin_row_idx);
		// 默认第一行是无效数据
		all_cell_values.emplace_back();
		for (std::uint32_t i = value_begin_row_idx; i < all_row_info.size(); i++)
		{
			all_cell_values.emplace_back(all_row_info[i].size());
		}
		for (std::uint32_t i = value_begin_row(); i < all_row_info.size(); i++)
		{
			for (std::uint32_t j = 1; j < all_row_info[i].size(); j++)
			{
				string_view cur_cell = get_cell(i, j);
				if (cur_cell.empty())
				{
					continue;
				}
				
				auto new_typed_value = memory_arena.get<arena_typed_value>(1);
				
				if (cur_parser.parse_value_with_type(typed_headers[j]->type_desc, cur_cell, *new_typed_value))
				{
					all_cell_values[i - value_begin_row_idx + 1][j] = new_typed_value;
				}
				
			}
			if (all_cell_values[i - value_begin_row_idx + 1][1]->type_desc)
			{
				_indexes[(all_cell_values[i - value_begin_row_idx + 1][1])] = i - value_begin_row_idx + 1;
			}
		}
	}
	typed_worksheet::typed_worksheet(const vector<cell>& all_cells, uint32_t in_sheet_id, string_view in_sheet_name, const workbook<typed_worksheet>* in_workbook)
	: worksheet(all_cells, in_sheet_id, in_sheet_name, reinterpret_cast<const workbook<worksheet>*>(in_workbook))
	, memory_arena(4 * 1024)
	{

	}
	const workbook<typed_worksheet>* typed_worksheet::get_workbook() const
	{
		return reinterpret_cast<const workbook<typed_worksheet>*>(_workbook);
	}
	void typed_worksheet::after_load_process()
	{
		if (convert_typed_header())
		{
			convert_cell_to_arena_typed_value();
		}
		else
		{
			for (auto i : typed_headers)
			{
				if (i)
				{
					delete i;
				}
			}
			typed_headers.clear();
			header_column_index.clear();
		}
		
	}
	typed_worksheet::~typed_worksheet()
	{
		for (auto i : typed_headers)
		{
			if (i)
			{
				delete i;
			}
		}
		

	}
	const vector<const typed_header*>& typed_worksheet::get_typed_headers() const
	{
		return typed_headers;
	}
	uint32_t typed_worksheet::get_header_idx(string_view header_name) const
	{
		auto header_iter = header_column_index.find(header_name);
		if (header_iter == header_column_index.end())
		{
			return 0;
		}
		else
		{
			return header_iter->second;
		}
	}
	uint32_t typed_worksheet::get_indexed_row(const arena_typed_value* first_row_value) const
	{
		auto iter = _indexes.find(first_row_value);
		if(iter == _indexes.end())
		{
			return 0;
		}
		else
		{
			return iter->second;
		}
	}
	const arena_typed_value* typed_worksheet::get_typed_cell_value(uint32_t row_idx, uint32_t column_idx) const
	{
		if (row_idx == 0 || column_idx == 0)
		{
			return nullptr;
		}
		if (row_idx >= all_cell_values.size())
		{
			return nullptr;
		}
		if (column_idx >= all_cell_values[row_idx].size())
		{
			return nullptr;
		}
		return all_cell_values[row_idx][column_idx];
	}
	const vector<const arena_typed_value*>& typed_worksheet::get_ref_row(string_view sheet_name, const arena_typed_value*  first_row_value) const
	{
		auto current_workbook = get_workbook();
		if(! current_workbook)
		{
			return all_cell_values[0];
		}
		auto sheet_idx = current_workbook->get_sheet_index_by_name(sheet_name);
		if(! sheet_idx)
		{
			return all_cell_values[0];
		}
		const auto& the_worksheet = current_workbook->get_worksheet(sheet_idx.value());
		auto row_index = the_worksheet.get_indexed_row(first_row_value);
		if(!row_index)
		{
			return all_cell_values[0];
		}
		return the_worksheet.get_typed_row(row_index);
	}
	const vector<const arena_typed_value*>& typed_worksheet::get_typed_row(uint32_t _idx) const
	{
		if (_idx == 0 || _idx >= all_cell_values.size())
		{
			return all_cell_values[0];
		}
		else
		{
			return all_cell_values[_idx];
		}
	}
	std::vector<std::reference_wrapper<const std::vector<const arena_typed_value*>>> typed_worksheet::get_all_typed_row_info() const
	{
		std::vector<std::reference_wrapper<const std::vector<const arena_typed_value*>>> result;
		for (std::size_t i = 1; i < all_cell_values.size(); i++)
		{
			const auto& one_row = all_cell_values[i];

			result.push_back(std::ref(one_row));
		}
		return result;
	}
	std::vector<std::reference_wrapper<const std::vector<const arena_typed_value*>>> typed_worksheet::get_typed_row_with_pred(std::function<bool(const std::vector<const arena_typed_value*>&)> pred) const
	{
		std::vector<std::reference_wrapper<const std::vector<const arena_typed_value*>>> result;
		for (std::size_t i = 1; i < all_cell_values.size(); i++)
		{
			const auto& one_row = all_cell_values[i];
			if (pred(one_row))
			{
				result.push_back(std::ref(one_row));
			}
		}
		return result;
	}
	bool typed_worksheet::check_header_match(const unordered_map<string_view, const typed_header*>& other_headers, string_view index_column_name) const
	{
		if (typed_headers.size() < 2)
		{
			std::cout << "current sheet doesnt has headers " << std::endl;
			return false;
		}
		if(typed_headers[1]->header_name != index_column_name)
		{
			cout << "index column name mismatch input " << index_column_name << " current " << typed_headers[1]->header_name << endl;
			return false;
		}
		for(const auto& i : other_headers)
		{
			if (!i.second)
			{
				cout << "missing type desc for header " << i.first << endl;
				return false;
			}
			auto header_idx = get_header_idx(i.second->header_name);
			if(header_idx == 0)
			{
				cout << " cant find header " << i.second->header_name << endl;
				return false;
			}
			if (header_idx >= typed_headers.size())
			{
				cout << " cant find header " << i.second->header_name << endl;
				return false;
			}
			if (!typed_headers[header_idx])
			{
				cout << " cant find header " << i.second->header_name << endl;
				return false;
			}
			if(!(*typed_headers[header_idx] == *i.second))
			{
				cout << "header type mismatch for  " << i.second->header_name << endl;
				return false;
			}

		}
		
		
		return true;
	}
	bool typed_header::operator==(const typed_header& other) const
	{
		if(header_name != other.header_name)
		{
			return false;
		}
		if(type_desc == other.type_desc)
		{
			return true;
		}
		if(!type_desc || !other.type_desc)
		{
			return false;
		}
		return *type_desc == *other.type_desc;

	}
	
	vector<uint32_t> typed_worksheet::get_header_index_vector(const vector<string_view>& header_names) const
	{
		vector<uint32_t> result;
		for (const auto& i : header_names)
		{
			result.push_back(get_header_idx(i));
		}
		return result;
	}
	std::uint32_t typed_worksheet::memory_consumption() const
	{
		return memory_arena.consumption() + worksheet::memory_consumption();
	}
}