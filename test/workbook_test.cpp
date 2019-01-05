﻿#include <xlsx_workbook.h>
#include <xlsx_typed_worksheet.h>
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;
using namespace xlsx_reader;

//int main(int argc, char** argv)
int main(void)
{
	//string file_name(argv[1]);
	string file_name = "../examples/xlsx_shape_test2.xlsx";
	int wait = 0;
	cout << "size of typed value is " << sizeof(typed_value) << endl;
	auto archive_content = make_shared<archive>(file_name);
	cin >> wait;
	workbook<typed_worksheet> current_workbook(archive_content);
	uint32_t workbook_memory = current_workbook.memory_details();
	
	cin >> wait;
	auto sheet_idx_opt = current_workbook.get_sheet_index_by_name("tile_1");
	const typed_worksheet& cur_worksheet = current_workbook.get_worksheet(sheet_idx_opt.value());
	vector<string_view> header_names = { "tile_id", "circle_id", "width", "sequence", "color", "ref_color", "opacity", "filled" };
	vector<uint32_t> header_indexes = cur_worksheet.get_header_index_vector(header_names);
	for (int i = 0; i < header_names.size(); i++)
	{
		std::cout << "head " << header_names[i] << " at " << header_indexes[i] << endl;
	}
	auto row_convert_result = cur_worksheet.try_convert_row<string_view, string_view, int, int, tuple<int, int, int>, string_view, float, bool>(1, header_indexes);
	auto [opt_tile_id, opt_circle_id, opt_width, opt_seq, opt_color, opt_ref_color, opt_opacity, opt_filled] = row_convert_result;
	return 0;

}