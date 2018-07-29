﻿#include <xlsx_typed.h>
#include <iostream>
namespace {
    using namespace xlsx_reader;
    using namespace std;

}
namespace xlsx_reader{
    using namespace std;
    typed_header::typed_header(const extend_node_type_descriptor* in_type_desc, string_view in_header_name, string_view in_header_comment):type_desc(in_type_desc), header_name(in_header_name), header_comment(in_header_comment)
    {

    }
    std::ostream& operator<<(std::ostream& output_stream, const typed_header& in_typed_header)
    {
        output_stream<<"name: "<< in_typed_header.header_name<<" type_desc: "<<*in_typed_header.type_desc<<" comment:"<<in_typed_header.header_comment<<endl;
        return output_stream;
    }
    void to_json(json& j, const typed_header& cur_typed_header)
    {
        auto new_j = json({{"name", cur_typed_header.header_name}, {"type_desc", *cur_typed_header.type_desc}, {"comment", cur_typed_header.header_comment}});
        j = new_j;
        return;
    }
    bool typed_worksheet::convert_typed_header()
    {
        int column_idx = 1;
        auto header_name_row = get_row(1);
        for(const auto& i: header_name_row)
        {
            if(i.first != column_idx)
            {
                cout<<"not continuous header row, current is "<< i.first<<" while expecting "<< column_idx<<endl;
                return false;
            }
            
            auto cur_cell_value = i.second;
            if(!cur_cell_value)
            {
                cerr<<"empty header name at idx "<<i.first<<endl;
                return false;
            }
            if(cur_cell_value->_type != cell_type::shared_string && cur_cell_value->_type != cell_type::inline_string)
            {
                cerr<<"invalid value "<<*cur_cell_value<<" for header name at column "<<i.first<<endl;
                return false;
            }
            auto cur_header_name = cur_cell_value->get_value<string_view>();
            cur_cell_value = get_cell(2, column_idx);
            if(!cur_cell_value)
            {
                cerr<<"empty cell type value for header "<< cur_header_name<<endl;
                return false;
            }
            if(cur_cell_value->_type != cell_type::shared_string && cur_cell_value->_type != cell_type::inline_string)
            {
                cerr<<"invalid value "<<*cur_cell_value<<" for header type at column "<<i.first<<endl;
                return false;
            }
            auto cur_type_desc = extend_node_value_constructor::parse_type(cur_cell_value->get_value<string_view>());
            if(!cur_type_desc)
            {
                cerr<<"invalid type desc "<<cur_cell_value->get_value<string_view>()<<"for header type at column "<<i.first<<endl;
                return false;
            }
            string_view header_comment;
            cur_cell_value = get_cell(3, column_idx);
            if(cur_cell_value)
            {
                header_comment = cur_cell_value->get_value<string_view>();
            }
            typed_headers.emplace_back(cur_type_desc, cur_header_name, header_comment);
            column_idx += 1;
        }
		return true;
    }
    int typed_worksheet::value_begin_row() const
    {
        return 4;
    }
    void typed_worksheet::convert_cell_to_typed_value()
    {
        typed_cells.clear();
        auto value_begin_row_idx = value_begin_row();
        for(const auto i: row_info)
        {
            if(i.first < value_begin_row_idx)
            {
                continue;
            }
            map<std::uint32_t, const typed_cell*> cur_row_typed_info;
            for(const auto& j: i.second)
            {
                if(!j.second)
                {
                    continue;
                }
                auto cur_typed_cell = extend_node_value_constructor::parse_node(typed_headers[j.first - 1].type_desc, j.second);
                if(!cur_typed_cell)
                {
                    continue;
                }
                cur_row_typed_info[j.first] = cur_typed_cell;
                typed_cells.push_back(*cur_typed_cell);
            }
            typed_row_info[i.first] = cur_row_typed_info;
        }
    }
    typed_worksheet::typed_worksheet(const vector<cell>& all_cells, uint32_t in_sheet_id, string_view in_sheet_name)
    : worksheet(all_cells, in_sheet_id, in_sheet_name)
    {

    }
    void typed_worksheet::after_load_process()
    {
        convert_typed_header();
        convert_cell_to_typed_value();
    }
    void to_json(json& j, const typed_worksheet& cur_worksheet)
    {
        json new_j;
        new_j["headers"] = json(cur_worksheet.typed_headers);
        new_j["sheet_id"] = cur_worksheet._sheet_id;
        new_j["sheet_name"] = cur_worksheet._name;
        json row_matrix;

        for(const auto& row_info: cur_worksheet.typed_row_info)
        {
            auto cur_row_index = row_info.first;
            json row_j = json::object();
            for(const auto& column_info: row_info.second)
            {
                row_j[to_string(column_info.first)] = json(*column_info.second->cur_typed_value);
            }
            row_matrix[to_string(cur_row_index)] = row_j;
        }
        new_j["matrix"] = row_matrix;
        j = new_j;
        return;
    }
	typed_worksheet::~typed_worksheet()
	{

	}
}