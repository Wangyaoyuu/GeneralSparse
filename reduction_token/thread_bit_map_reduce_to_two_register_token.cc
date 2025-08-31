// #include "../reduction_token.hpp"
// #include "../IO_of_reduction.hpp"
// #include "../code_generator.hpp"

// thread_bit_map_reduce_to_two_register_token::thread_bit_map_reduce_to_two_register_token(shared_ptr<meta_data_set> meta_data_set_ptr, bool need_warp_reduction, unsigned int coarsen_factor, shared_ptr<code_generator> code_generator_ptr)
//     : reduction_basic_token(THREAD_META, meta_data_set_ptr)
// {
//     assert(meta_data_set_ptr != NULL);
//     assert(code_generator_ptr != NULL);
//     assert(code_generator_ptr->check() == true);

//     this->code_generator_ptr = code_generator_ptr;
//     this->need_warp_reduction = need_warp_reduction;
//     this->coarsen_factor = coarsen_factor;

//     int count = coarsen_factor;

//     // input IO默认设计空的
//     // output IO需要一个对应位置的输出
//     shared_ptr<two_register_result_IO_of_reduction> two_register_IO(new two_register_result_IO_of_reduction(THREAD_META, count));
//     this->output_IO = two_register_IO;
// }

// // 执行对应的代码生成过程
// string thread_bit_map_reduce_to_two_register_token::run()
// {
//     // 将代码生成器复原成智能指针
//     shared_ptr<code_generator> code_generator_ptr = this->code_generator_ptr.lock();

//     if (code_generator_ptr == NULL)
//     {
//         cout << "thread_bit_map_reduce_to_two_register_token::run(): the code generator has already destroyed" << endl;
//         assert(false);
//     }

//     assert(code_generator_ptr->check() == true);

//     stringstream return_code_str;

//     string result_register_name_1 = dynamic_pointer_cast<two_register_result_IO_of_reduction>(this->output_IO)->var_names_token_of_IO_register()[0]->run();
//     string result_register_name_2 = dynamic_pointer_cast<two_register_result_IO_of_reduction>(this->output_IO)->var_names_token_of_IO_register()[1]->run();

//     // 查看当前是不是交错存储，如果没有交错存储，就直接遍历这个BMT的对应的所有数据块

//     // 查看当前所针对的子矩阵
//     int sub_matrix_id = code_generator_ptr->get_sub_matrix_id();

//     assert(sub_matrix_id >= 0);

//     assert(this->meta_data_set_ptr->is_exist(GLOBAL_META, "nz_vals", sub_matrix_id) == true);
//     shared_ptr<universal_array> var_array_ptr = this->meta_data_set_ptr
//                                                     ->get_element(GLOBAL_META, "nz_vals", sub_matrix_id)
//                                                     ->get_metadata_arr();

//     data_type data_type_of_result = var_array_ptr->get_data_type();

//     if (data_type_of_result == FLOAT && get_config()["HALF"].as_bool() == true)
//     {
//         data_type_of_result = HALF;
//     }

//     shared_ptr<math_expr_token> dense_matrix_ptr_init = NULL;
//     shared_ptr<math_expr_token> dense_matrix_step = NULL;

//     if (need_warp_reduction == false)
//     {
//         dense_matrix_ptr_init = make_shared<math_expr_token>("(blockIdx.y * blockDim.x) + threadIdx.x");
//         dense_matrix_step = make_shared<math_expr_token>("blockDim.x * gridDim.y * " + to_string(this->coarsen_factor));
//     }
//     else
//     {
//         string dense_ = "((blockIdx.y * blockDim.y) + threadIdx.y) * " + to_string(this->coarsen_factor);
//         dense_matrix_ptr_init = make_shared<math_expr_token>(dense_);
//         dense_matrix_step = make_shared<math_expr_token>("blockDim.y * gridDim.y * " + to_string(this->coarsen_factor));
//     }

//     shared_ptr<math_expr_token> zero_(new math_expr_token("0"));
//     shared_ptr<var_name_token> result_register_1 = code_generator_ptr->generate_global_var(data_type_of_result, result_register_name_1, zero_, this->coarsen_factor);
//     shared_ptr<var_name_token> result_register_2 = code_generator_ptr->generate_global_var(data_type_of_result, result_register_name_2, zero_, this->coarsen_factor);
   
//     shared_ptr<var_name_token> tmp_result = code_generator_ptr->generate_global_var(data_type_of_result, "temp_result", zero_, this->coarsen_factor);

//     shared_ptr<var_name_token> dense_matrix_ptr = code_generator_ptr->generate_global_var(UNSIGNED_INT, "dense_matrix_ptr", dense_matrix_ptr_init);

//     shared_ptr<var_name_token> tmp_row = code_generator_ptr->generate_global_var(UNSIGNED_LONG, "tmp_row", NULL);

//     shared_ptr<var_name_token> first_row_thread = code_generator_ptr->generate_global_var(UNSIGNED_LONG, "first_row_thread", NULL);

//     shared_ptr<var_name_token> segment_offset = code_generator_ptr->generate_global_var(UNSIGNED_LONG, "global_segment_offset", NULL);
//     shared_ptr<math_expr_token> lane_id_init;

//     if(this->need_warp_reduction == true)
//     {
//         lane_id_init = make_shared<math_expr_token>("threadIdx.x % 32");
//     }else
//     {        
//         lane_id_init = make_shared<math_expr_token>("(blockIdx.x * blockDim.y + threadIdx.y) % 32");
//     }
//     shared_ptr<var_name_token> lane_id = code_generator_ptr->generate_global_var(UNSIGNED_INT, "lane_id", lane_id_init);

//     shared_ptr<math_expr_token> fls(new math_expr_token("false"));
//     shared_ptr<var_name_token> prev_flag = code_generator_ptr->generate_global_var(BOOL, "prev_flag", fls);
//     shared_ptr<var_name_token> cur_flag = code_generator_ptr->generate_global_var(BOOL, "cur_flag", fls);


//     shared_ptr<math_expr_token> cur_BMT_id_token(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(THREAD_META)->run()));
//     shared_ptr<math_expr_token> next_BMT_id_token(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(THREAD_META)->run() + "+1"));

//     shared_ptr<basic_token> for_begin_var;
//     shared_ptr<basic_token> for_end_var;
//     string nz_step;
//     string BMT_nz_id = "BMT_nz_id";


//     // 列索引必须存在
//     assert(this->meta_data_set_ptr->is_exist(GLOBAL_META, "begin_col_index", sub_matrix_id) == true);

//     // 查看当前子矩阵的首列的列索引
//     unsigned long first_col_index_of_sub_matrix = this->meta_data_set_ptr
//                                                       ->get_element(GLOBAL_META, "begin_col_index", sub_matrix_id)
//                                                       ->get_metadata_arr()
//                                                       ->read_integer_from_arr(0);
//     shared_ptr<math_expr_token> col_and_val_read_index_token_ptr(new math_expr_token(BMT_nz_id));
//     vector<shared_ptr<basic_token>> col_indices_read_token_vec;
//     vector<shared_ptr<basic_token>> vals_read_token_vec;



//     if (code_generator_ptr->get_interleave_storage() == true)
//     {
//         col_indices_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_col_indices_after_interlance_storage", col_and_val_read_index_token_ptr, false);
//         vals_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_vals_after_interlance_storage", col_and_val_read_index_token_ptr, false);
//         shared_ptr<basic_token> bmt_size;
//         if (this->meta_data_set_ptr->is_exist(WARP_META, "first_nz_indices", sub_matrix_id) == true)
//         {
//             shared_ptr<math_expr_token> warp_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(WARP_META)->run()));
//             shared_ptr<math_expr_token> next_warp_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(WARP_META)->run() + "+1"));
//             bmt_size = code_generator_ptr->generate_fused_memory_access(WARP_META, "BMT_size_of_each_blk", warp_id);
//             shared_ptr<var_name_token> first_bmt_id = code_generator_ptr->generate_fused_memory_access_with_relative(WARP_META, "first_BMT_indices", warp_id);
//             shared_ptr<var_name_token> first_bmt_id_next = code_generator_ptr->generate_fused_memory_access_with_relative(WARP_META, "first_BMT_indices", next_warp_id);
//             shared_ptr<var_name_token> first_nz_indices_BMW = code_generator_ptr->generate_fused_memory_access_with_relative(WARP_META, "first_nz_indices", warp_id);
//             string begin_math_expr = first_nz_indices_BMW->run() + " + " + cur_BMT_id_token->run() + " - " + first_bmt_id->run();
//             string end_math_expr = begin_math_expr + " + " + bmt_size->run() + " * (" + first_bmt_id_next->run() + " - " + first_bmt_id->run() + ")";
//             for_begin_var = make_shared<math_expr_token>(begin_math_expr);
//             for_end_var = make_shared<math_expr_token>(end_math_expr);
//             nz_step = first_bmt_id_next->run() + " - " + first_bmt_id->run();            
//         }
//         else if (this->meta_data_set_ptr->is_exist(TBLOCK_META, "first_nz_indices", sub_matrix_id) == true)
//         {
//             shared_ptr<math_expr_token> block_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(TBLOCK_META)->run()));
//             shared_ptr<math_expr_token> next_block_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(TBLOCK_META)->run() + "+1"));
//             bmt_size = code_generator_ptr->generate_fused_memory_access(TBLOCK_META, "BMT_size_of_each_blk", block_id);
//             shared_ptr<var_name_token> first_bmt_id = code_generator_ptr->generate_fused_memory_access_with_relative(TBLOCK_META, "first_BMT_indices", block_id);
//             shared_ptr<var_name_token> first_bmt_id_next = code_generator_ptr->generate_fused_memory_access_with_relative(TBLOCK_META, "first_BMT_indices", next_block_id);
//             shared_ptr<var_name_token> first_nz_indices_BMTB = code_generator_ptr->generate_fused_memory_access_with_relative(TBLOCK_META, "first_nz_indices", block_id);
//             string begin_math_expr = first_nz_indices_BMTB->run() + " + " + cur_BMT_id_token->run() + " - " + first_bmt_id->run();
//             string end_math_expr = begin_math_expr + " + " + bmt_size->run() + " * (" + first_bmt_id_next->run() + " - " + first_bmt_id->run() + ")";
//             for_begin_var = make_shared<math_expr_token>(begin_math_expr);
//             for_end_var = make_shared<math_expr_token>(end_math_expr);
//             nz_step = first_bmt_id_next->run() + " - " + first_bmt_id->run();
//         }
//         else
//         {
//             shared_ptr<math_expr_token> zero_id(new math_expr_token("0"));
//             unsigned int BMT_num = this->meta_data_set_ptr->get_element(THREAD_META, "first_nz_indices", sub_matrix_id)->get_metadata_arr()->get_len() - 1;
//             bmt_size = code_generator_ptr->generate_fused_memory_access(GLOBAL_META, "BMT_size_of_each_blk", zero_id);
//             for_begin_var = make_shared<math_expr_token>(cur_BMT_id_token->run());
//             for_end_var = make_shared<math_expr_token>(cur_BMT_id_token->run() + " + " + bmt_size->run() + " * " + to_string(BMT_num));
//             nz_step = to_string(BMT_num);
//         }
//     }
//     else
//     {
//         col_indices_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_col_indices", col_and_val_read_index_token_ptr, false);
//         vals_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_vals", col_and_val_read_index_token_ptr, false);
//         for_begin_var = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "first_nz_indices", cur_BMT_id_token);
//         for_end_var = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "first_nz_indices", next_BMT_id_token);
//         nz_step = "1";
//     }

//     string col_indices_read_name = dynamic_pointer_cast<var_init_token>(col_indices_read_token_vec[0])->get_inited_var_name();
//     string var_read_name = dynamic_pointer_cast<var_init_token>(vals_read_token_vec[0])->get_inited_var_name();

//     data_type nz_data_type = this->meta_data_set_ptr->get_element(THREAD_META, "first_nz_indices", sub_matrix_id)
//                                  ->get_metadata_arr()
//                                  ->get_data_type();

    
//     shared_ptr<math_expr_token> empty_flag_access(new math_expr_token(cur_BMT_id_token->run() + " / 32"));
//     shared_ptr<basic_token> seg_ptr = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "segment_ptr", cur_BMT_id_token);
//     shared_ptr<basic_token> seg_flag = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "segment_empty_flag", empty_flag_access);
//     shared_ptr<basic_token> thread_bit_map = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "thread_bit_map", cur_BMT_id_token);
//     shared_ptr<basic_token> thread_row_indices =  code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "first_row_indices", cur_BMT_id_token);

//     string dense_ptr = dense_matrix_ptr->run();

//     if (this->need_warp_reduction == true)
//     {

//         unsigned int r_size = 1;
//         unsigned int a_size = 4;

//         if (this->coarsen_factor == 2)
//         {
//             if (data_type_of_result == FLOAT)
//             {
//                 r_size = 1;
//                 a_size = 2;
//             }
//             else if (data_type_of_result == DOUBLE)
//             {
//                 r_size = 2;
//             }
            
//         }

//         if (data_type_of_result == HALF)
//         {
//             r_size = 1;
//             a_size = 2;
//         }
        
//         string vec_acc = "float";
//         if(data_type_of_result == HALF)
//         {
//             vec_acc = "half";
//         }

//         return_code_str << code_of_data_type(data_type_of_result) << " buffer[" << to_string(this->coarsen_factor) << "];" << endl;
//         // return_code_str << "for (unsigned int p = 0; p <" << to_string(this->coarsen_factor) << "; p++)" << endl;
//         // return_code_str << "{" << endl;
//         // return_code_str << result_register_name_1 << "[p] = 0;" << endl;
//         // return_code_str << result_register_name_2 << "[p] = 0;" << endl;
//         // return_code_str << "temp_result[p] = 0;" << endl;

//         // return_code_str << "}" << endl;

//         return_code_str << "first_row_thread = " << thread_row_indices->run() << ";" << endl;
//         return_code_str << "tmp_row = first_row_thread;" << endl;
//         return_code_str << "unsigned int count = 0;" << endl;

//         return_code_str << "if(" << dense_ptr << " + " << to_string(this->coarsen_factor) << " <= K)" << endl;
//         return_code_str << "{" << endl;


//         return_code_str << "for (" << code_of_data_type(nz_data_type) << " " << BMT_nz_id << " = ";
//         return_code_str << for_begin_var->run() << "; " << BMT_nz_id << " < " << for_end_var->run() << "; ";
//         return_code_str << BMT_nz_id << "+= " << nz_step << ")" << endl;

//         return_code_str << "{" << endl;

//         for (unsigned int i = 0; i < col_indices_read_token_vec.size(); i++)
//         {
//             return_code_str << col_indices_read_token_vec[i]->run() << endl;
//         }

//         for (unsigned int i = 0; i < vals_read_token_vec.size(); i++)
//         {
//             return_code_str << vals_read_token_vec[i]->run() << endl;
//         }

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int q = 0; q < " << to_string(this->coarsen_factor * r_size / a_size) << "; q++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "*(" << vec_acc << to_string(a_size) << " *)(buffer + q * " << to_string(a_size / r_size) << ") = *(" << vec_acc << to_string(a_size) << " *)";
//             if (first_col_index_of_sub_matrix != 0)
//             {
//                 return_code_str << "(x_arr + " << dense_ptr << " + (" << col_indices_read_name << " + " << first_col_index_of_sub_matrix << ") * K + q * " << to_string(a_size / r_size) << ");" << endl;
//             }
//             else
//             {
//                 return_code_str << "(x_arr +" << dense_ptr << " + " << col_indices_read_name << " * K + q * " << to_string(a_size / r_size) << ");" << endl;
//             }
//             return_code_str << "}" << endl;
//         }
//         else
//         {

//             return_code_str << "buffer[0]" << " = ";
//             if (first_col_index_of_sub_matrix != 0)
//             {
//                 return_code_str << "x_arr[" << dense_ptr << " + (" << col_indices_read_name << " + " << first_col_index_of_sub_matrix << ") * K];" << endl;
//             }
//             else
//             {
//                 return_code_str << "x_arr [" << dense_ptr << " + " << col_indices_read_name << " * K];" << endl;
//             }
//         }

//         return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
//         return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;


//         return_code_str << "if(cur_flag == true && BMT_nz_id !=" << for_begin_var->run() << "){" << endl;
//         return_code_str << "count += 1;" << endl;

//         return_code_str << "if(prev_flag == true){" << endl;

//         vector<shared_ptr<basic_token>> first_row_indices_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
//         string row_name = dynamic_pointer_cast<var_init_token>(first_row_indices_of_thread[0])->get_inited_var_name();
//         for (int i = 0; i < first_row_indices_of_thread.size(); i++)
//         {
//             return_code_str << first_row_indices_of_thread[i]->run() << endl;
//         }
//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr + r], temp_result[r]);" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr], temp_result);" << endl;
//         }

//         return_code_str << "}" << endl;
//         return_code_str << "else{" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << result_register_1->run() << " = " << "temp_result;" << endl;
//         }

//         return_code_str << "}" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for (unsigned int p = 0; p <" << to_string(this->coarsen_factor) << "; p++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "temp_result[p] = 0;" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << "temp_result = 0;" << endl;
//         }

//         return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << "){" << endl;
//         return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;
//         shared_ptr<math_expr_token> empty_id(new math_expr_token("empty_id"));
//         vector<shared_ptr<basic_token>> empty_row = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "segment_empty_row_indices", empty_id, false, "");
//         string empty_row_name = dynamic_pointer_cast<var_init_token>(empty_row[0])->get_inited_var_name();

//         for (int i = 0; i < empty_row.size(); i++)
//         {
//             return_code_str << empty_row[i]->run() << endl;
//         }

//         return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";"<< endl;

//         return_code_str << "}" << endl;
//         return_code_str << "else{" << endl;

//         return_code_str << "tmp_row = tmp_row + 1;" << endl; 

//         return_code_str << "}" << endl;

//         return_code_str << "}" << endl;
//         return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "temp_result[r] += " << var_read_name << " * "
//                             << "buffer[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << "temp_result += " << var_read_name << " * "
//                             << "buffer[0];" << endl;
//         }

//         return_code_str << "}" << endl;


//         return_code_str << "if(prev_flag == false){" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r < " << to_string(this->coarsen_factor) << "; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << result_register_1->run() << " = " << "temp_result;" << endl;
//         }

//         return_code_str << "}" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << result_register_2->run() << "[r] = " << "temp_result[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << result_register_2->run() << " = " << "temp_result;" << endl;
//         }

//         return_code_str << "}" << endl;
//         return_code_str << "else{" << endl;

//         return_code_str << "if(" << dense_ptr << " >= K)"<< endl;
//         return_code_str << "return;" << endl;

//         return_code_str << "unsigned int size_of_buffer = K - " << dense_ptr << ";" << endl;

//         // for循环
//         return_code_str << "for (" << code_of_data_type(nz_data_type) << " " << BMT_nz_id << " = ";
//         return_code_str << for_begin_var->run() << "; " << BMT_nz_id << " < " << for_end_var->run() << "; ";
//         return_code_str << BMT_nz_id << "+= " << nz_step << ")" << endl;

//         return_code_str << "{" << endl;
//         for (unsigned int i = 0; i < col_indices_read_token_vec.size(); i++)
//         {
//             return_code_str << col_indices_read_token_vec[i]->run() << endl;
//         }

//         for (unsigned int i = 0; i < vals_read_token_vec.size(); i++)
//         {
//             return_code_str << vals_read_token_vec[i]->run() << endl;
//         }

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int q = 0; q < size_of_buffer; q++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "buffer[q]"
//                             << " = ";
//             if (first_col_index_of_sub_matrix != 0)
//             {
//                 return_code_str << "x_arr[" << dense_ptr << " + (" << col_indices_read_name << " + " << first_col_index_of_sub_matrix << ") * K + q];" << endl;
//             }
//             else
//             {
//                 return_code_str << "x_arr[" << dense_ptr << " + " << col_indices_read_name << " * K + q];" << endl;
//             }
//             return_code_str << "}" << endl;
//         }
//         else
//         {

//             return_code_str << "buffer[0]"
//                             << " = ";
//             if (first_col_index_of_sub_matrix != 0)
//             {
//                 return_code_str << "x_arr[" << dense_ptr << " + (" << col_indices_read_name << " + " << first_col_index_of_sub_matrix << ") * K];" << endl;
//             }
//             else
//             {
//                 return_code_str << "x_arr[" << dense_ptr << " + " << col_indices_read_name << " * K];" << endl;
//             }
//         }

//         return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
//         return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;


//         return_code_str << "if(cur_flag == true && BMT_nz_id !=" << for_begin_var->run() << "){" << endl;
//         return_code_str << "count += 1;" << endl;


//         return_code_str << "if(prev_flag == true){" << endl;

//         for (int i = 0; i < first_row_indices_of_thread.size(); i++)
//         {
//             return_code_str << first_row_indices_of_thread[i]->run() << endl;
//         }

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr + r], temp_result[r]);" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr], temp_result);" << endl;
//         }

//         return_code_str << "}" << endl;
//         return_code_str << "else{" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << result_register_1->run() << " = " << "temp_result;" << endl;
//         }

//         return_code_str << "}" << endl;
//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for (unsigned int p = 0; p < size_of_buffer; p++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "temp_result[p] = 0;" << endl;
//             return_code_str << "}" << endl;
//         }else
//         {
//             return_code_str << "temp_result = 0;" << endl;
//         }

//         return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << "){" << endl;
//         return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;

//         for (int i = 0; i < empty_row.size(); i++)
//         {
//             return_code_str << empty_row[i]->run() << endl;
//         }

//         return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";" << endl;

//         return_code_str << "}" << endl;
//         return_code_str << "else{" << endl;

//         return_code_str << "tmp_row = tmp_row + 1;" << endl; 

//         return_code_str << "}" << endl;

//         return_code_str << "}" << endl;
//         return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "temp_result[r] += " << var_read_name << " * "
//                             << "buffer[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << "temp_result += " << var_read_name << " * "
//                             << "buffer[0];" << endl;
//         }

//         return_code_str << "}" << endl;

//         return_code_str << "if(prev_flag == false){" << endl;
//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << result_register_1->run() << " = " << "temp_result;" << endl;
//         }

//         return_code_str << "}" << endl;

//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << result_register_2->run() << "[r] = " << "temp_result[r];" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << result_register_2->run() << " = " << "temp_result;" << endl;
//         }

//         return_code_str << "}" << endl;
//         shared_ptr<var_name_token> segment_offset_fuse = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "segment_offset", cur_BMT_id_token);
//         return_code_str << segment_offset->run() << " = " << segment_offset_fuse->run() << ";" << endl;
//     }
//     else
//     {
//         if (this->coarsen_factor > 1)
//         {
//             return_code_str << "for (unsigned int p = 0; p <" << to_string(this->coarsen_factor) << "; p++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << result_register_name_1 << "[p] = 0;" << endl;
//             return_code_str << result_register_name_2 << "[p] = 0;" << endl;
//             return_code_str << "temp_result[p] = 0;" << endl;
//             return_code_str << "}" << endl;
//         }
//         else
//         {
//             return_code_str << result_register_name_1 << " = 0;" << endl;
//             return_code_str << result_register_name_2 << " = 0;" << endl;
//             return_code_str << "temp_result = 0;" << endl;        
//         }

//         return_code_str << "bool prev_flag = false;" << endl;
//         return_code_str << "bool cur_flag = false;" << endl;
//         return_code_str << "first_row_thread = " << thread_row_indices->run() << ";" << endl;
//         return_code_str << "tmp_row = first_row_thread;" << endl;
//         return_code_str << "unsigned int count = 0;" << endl;

//         // for循环
//         return_code_str << "for (" << code_of_data_type(nz_data_type) << " " << BMT_nz_id << " = ";
//         return_code_str << for_begin_var->run() << "; " << BMT_nz_id << " < " << for_end_var->run() << "; ";
//         return_code_str << BMT_nz_id << "+= " << nz_step << ")" << endl;

//         return_code_str << "{" << endl;

//         for (unsigned int i = 0; i < col_indices_read_token_vec.size(); i++)
//         {
//             return_code_str << col_indices_read_token_vec[i]->run() << endl;
//         }

//         for (unsigned int i = 0; i < vals_read_token_vec.size(); i++)
//         {
//             return_code_str << vals_read_token_vec[i]->run() << endl;
//         }

//         if (this->coarsen_factor > 1)
//         {

//             return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
//             return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;

//             return_code_str << "if(cur_flag == true && BMT_nz_id !=" << for_begin_var->run() << "){" << endl;
//             return_code_str << "count += 1;" << endl;

           

//             vector<shared_ptr<basic_token>> cur_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
//             string row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
//             for (int i = 0; i < cur_row_of_thread.size(); i++)
//             {
//                 return_code_str << cur_row_of_thread[i]->run() << endl;
//             }

//             return_code_str << "for (int r = " << dense_ptr << ", c = 0; r < K && c < " << this->coarsen_factor << "; r += blockDim.x * gridDim.y, c++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + r], temp_result[c]);" << endl;
//             return_code_str << "}" << endl;

//             return_code_str << "for (unsigned int p = 0; p <" << to_string(this->coarsen_factor) << "; p++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "temp_result[p] = 0;" << endl;
//             return_code_str << "}" << endl;

//             return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << ")"
//                             << "{" << endl;
//             return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;
//             shared_ptr<math_expr_token> empty_id(new math_expr_token("empty_id"));
//             vector<shared_ptr<basic_token>> empty_row = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "segment_empty_row_indices", empty_id, false, "");
//             string empty_row_name = dynamic_pointer_cast<var_init_token>(empty_row[0])->get_inited_var_name();

//             for (int i = 0; i < empty_row.size(); i++)
//             {
//                 return_code_str << empty_row[i]->run() << endl;
//             }

//             return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";" << endl;

//             return_code_str << "}" << endl;
//             return_code_str << "else{" << endl;

//             return_code_str << "tmp_row = tmp_row + 1;" << endl;

//             return_code_str << "}" << endl;

//             return_code_str << "}" << endl;
//             return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;

           
//             return_code_str << "for (int r = " << dense_ptr << ", c = 0; r < K && c < " << this->coarsen_factor << "; r += blockDim.x * gridDim.y, c++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "temp_result[c] += " << var_read_name << " * ";
//             if (first_col_index_of_sub_matrix != 0)
//             {
//                 return_code_str << "__ldg(&(x_arr[r + (" << col_indices_read_name << " + " << first_col_index_of_sub_matrix << ") * K]));" << endl;
//             }
//             else
//             {
//                 return_code_str << "__ldg(&(x_arr[r + " << col_indices_read_name << " * K]));" << endl;
//             }
//             return_code_str << "}" << endl;

//             return_code_str << "}" << endl;

//             vector<shared_ptr<basic_token>> last_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
//             string last_row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
//             for (int i = 0; i < cur_row_of_thread.size(); i++)
//             {
//                 return_code_str << cur_row_of_thread[i]->run() << endl;
//             }

//             return_code_str << "for (int r = " << dense_ptr << ", c = 0; r < K && c < " << this->coarsen_factor << "; r += blockDim.x * gridDim.y, c++)" << endl;
//             return_code_str << "{" << endl;
//             return_code_str << "atomicAdd(&y_arr[" << last_row_name << " * K + r], temp_result[c]);" << endl;

//             return_code_str << "}" << endl;


//         }
//         else
//         {

//            return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
//             return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;

//             return_code_str << "if(cur_flag == true && BMT_nz_id !=" << for_begin_var->run() << "){" << endl;
           
//             return_code_str << "count += 1;" << endl;

//             vector<shared_ptr<basic_token>> cur_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
//             string row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
//             for (int i = 0; i < cur_row_of_thread.size(); i++)
//             {
//                 return_code_str << cur_row_of_thread[i]->run() << endl;
//             }


//             return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr], temp_result);" << endl;
//             return_code_str << "temp_result = 0;" << endl;

//             return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << "){" << endl;
//             return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;
//             shared_ptr<math_expr_token> empty_id(new math_expr_token("empty_id"));
//             vector<shared_ptr<basic_token>> empty_row = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "segment_empty_row_indices", empty_id, false, "");
//             string empty_row_name = dynamic_pointer_cast<var_init_token>(empty_row[0])->get_inited_var_name();

//             for (int i = 0; i < empty_row.size(); i++)
//             {
//                 return_code_str << empty_row[i]->run() << endl;
//             }

//             return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";" << endl;

//             return_code_str << "}" << endl;
//             return_code_str << "else{" << endl;

//             return_code_str << "tmp_row = tmp_row + 1;" << endl;

//             return_code_str << "}" << endl;

//             return_code_str << "}" << endl;
//             return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;

           
//             return_code_str << "temp_result += " << var_read_name << " * ";
//             if (first_col_index_of_sub_matrix != 0)
//             {
//                 return_code_str << "__ldg(&(x_arr[dense_matrix_ptr + (" << col_indices_read_name << " + " << first_col_index_of_sub_matrix << ") * K]));" << endl;
//             }
//             else
//             {
//                 return_code_str << "__ldg(&(x_arr[dense_matrix_ptr + " << col_indices_read_name << " * K]));" << endl;
//             }

//             return_code_str << "}" << endl;

//             vector<shared_ptr<basic_token>> last_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
//             string last_row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
//             for (int i = 0; i < cur_row_of_thread.size(); i++)
//             {
//                 return_code_str << cur_row_of_thread[i]->run() << endl;
//             }
//             return_code_str << "atomicAdd(&y_arr[" << last_row_name << " * K + dense_matrix_ptr], temp_result);" << endl;


//         }

//     }

//     return return_code_str.str();
// }

// shared_ptr<basic_IO_of_reduction>thread_bit_map_reduce_to_two_register_token::get_output_IO()
// {
//     return this->output_IO;
// }


#include "../reduction_token.hpp"
#include "../IO_of_reduction.hpp"
#include "../code_generator.hpp"

thread_bit_map_reduce_to_two_register_token::thread_bit_map_reduce_to_two_register_token(shared_ptr<meta_data_set> meta_data_set_ptr, bool need_warp_reduction, unsigned int sparse_coarsen_factor, unsigned int coarsen_factor, shared_ptr<code_generator> code_generator_ptr)
    : reduction_basic_token(THREAD_META, meta_data_set_ptr)
{
    assert(meta_data_set_ptr != NULL);
    assert(code_generator_ptr != NULL);
    assert(code_generator_ptr->check() == true);

    this->code_generator_ptr = code_generator_ptr;
    this->need_warp_reduction = need_warp_reduction;
    this->coarsen_factor = coarsen_factor;
    this->sparse_coarsen_factor = sparse_coarsen_factor;

    int count = coarsen_factor;

    // input IO默认设计空的
    // output IO需要一个对应位置的输出
    shared_ptr<two_register_result_IO_of_reduction> two_register_IO(new two_register_result_IO_of_reduction(THREAD_META, count));
    this->output_IO = two_register_IO;
}

// 执行对应的代码生成过程
string thread_bit_map_reduce_to_two_register_token::run()
{
    // 将代码生成器复原成智能指针
    shared_ptr<code_generator> code_generator_ptr = this->code_generator_ptr.lock();

    if (code_generator_ptr == NULL)
    {
        cout << "thread_bit_map_reduce_to_two_register_token::run(): the code generator has already destroyed" << endl;
        assert(false);
    }

    assert(code_generator_ptr->check() == true);

    stringstream return_code_str;

    string result_register_name_1 = dynamic_pointer_cast<two_register_result_IO_of_reduction>(this->output_IO)->var_names_token_of_IO_register()[0]->run();
    string result_register_name_2 = dynamic_pointer_cast<two_register_result_IO_of_reduction>(this->output_IO)->var_names_token_of_IO_register()[1]->run();

    // 查看当前是不是交错存储，如果没有交错存储，就直接遍历这个BMT的对应的所有数据块

    // 查看当前所针对的子矩阵
    int sub_matrix_id = code_generator_ptr->get_sub_matrix_id();

    assert(sub_matrix_id >= 0);

    assert(this->meta_data_set_ptr->is_exist(GLOBAL_META, "nz_vals", sub_matrix_id) == true);
    shared_ptr<universal_array> var_array_ptr = this->meta_data_set_ptr
                                                    ->get_element(GLOBAL_META, "nz_vals", sub_matrix_id)
                                                    ->get_metadata_arr();

    data_type data_type_of_result = var_array_ptr->get_data_type();

    data_type nz_data_type = this->meta_data_set_ptr->get_element(THREAD_META, "first_nz_indices", sub_matrix_id)
                                 ->get_metadata_arr()
                                 ->get_data_type();

    data_type nz_data_type_compress = this->meta_data_set_ptr->get_element(GLOBAL_META, "nz_col_indices", sub_matrix_id)
                                 ->get_metadata_arr()
                                 ->get_compress_data_type();



    if (data_type_of_result == FLOAT && get_config()["HALF"].as_bool() == true)
    {
        data_type_of_result = HALF;
    }

    shared_ptr<math_expr_token> dense_matrix_ptr_init = NULL;
    shared_ptr<math_expr_token> dense_matrix_step = NULL;

    if (need_warp_reduction == false)
    {
        dense_matrix_ptr_init = make_shared<math_expr_token>("((blockIdx.y * blockDim.x) + threadIdx.x) * " + to_string(this->coarsen_factor));
        dense_matrix_step = make_shared<math_expr_token>("blockDim.x * gridDim.y * " + to_string(this->coarsen_factor));
    }
    else
    {
        string dense_ = "((blockIdx.y * blockDim.y) + threadIdx.y) * " + to_string(this->coarsen_factor);
        dense_matrix_ptr_init = make_shared<math_expr_token>(dense_);
        dense_matrix_step = make_shared<math_expr_token>("blockDim.y * gridDim.y * " + to_string(this->coarsen_factor));
    }

    shared_ptr<math_expr_token> zero_(new math_expr_token("0"));
    shared_ptr<var_name_token> result_register_1 = code_generator_ptr->generate_global_var(data_type_of_result, result_register_name_1, zero_, this->coarsen_factor);
    shared_ptr<var_name_token> result_register_2 = code_generator_ptr->generate_global_var(data_type_of_result, result_register_name_2, zero_, this->coarsen_factor);
   
    shared_ptr<var_name_token> tmp_result = code_generator_ptr->generate_global_var(data_type_of_result, "temp_result", zero_, this->coarsen_factor);

    shared_ptr<var_name_token> dense_matrix_ptr = code_generator_ptr->generate_global_var(UNSIGNED_INT, "dense_matrix_ptr", dense_matrix_ptr_init);

    shared_ptr<var_name_token> tmp_row = code_generator_ptr->generate_global_var(UNSIGNED_LONG, "tmp_row", NULL);

    shared_ptr<var_name_token> first_row_thread = code_generator_ptr->generate_global_var(UNSIGNED_LONG, "first_row_thread", NULL);

    shared_ptr<var_name_token> segment_offset = code_generator_ptr->generate_global_var(UNSIGNED_LONG, "global_segment_offset", NULL);
    shared_ptr<math_expr_token> lane_id_init;

    if(this->need_warp_reduction == true)
    {
        lane_id_init = make_shared<math_expr_token>("threadIdx.x % " + to_string((int)get_config()["VECTOR_WIDTH"].as_float()));
    }else
    {        
        lane_id_init = make_shared<math_expr_token>("(blockIdx.x * blockDim.y + threadIdx.y) % " + to_string((int)get_config()["VECTOR_WIDTH"].as_float()));
    }
    shared_ptr<var_name_token> lane_id = code_generator_ptr->generate_global_var(UNSIGNED_INT, "lane_id", lane_id_init);

    shared_ptr<math_expr_token> fls(new math_expr_token("false"));
    shared_ptr<var_name_token> prev_flag = code_generator_ptr->generate_global_var(BOOL, "prev_flag", fls);
    shared_ptr<var_name_token> cur_flag = code_generator_ptr->generate_global_var(BOOL, "cur_flag", fls);


    shared_ptr<math_expr_token> cur_BMT_id_token(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(THREAD_META)->run()));
    shared_ptr<math_expr_token> next_BMT_id_token(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(THREAD_META)->run() + "+1"));

    shared_ptr<basic_token> for_begin_var;
    shared_ptr<basic_token> for_end_var;
    string nz_step;
    string BMT_nz_id = "BMT_nz_id";


    // 列索引必须存在
    assert(this->meta_data_set_ptr->is_exist(GLOBAL_META, "begin_col_index", sub_matrix_id) == true);

    // 查看当前子矩阵的首列的列索引
    unsigned long first_col_index_of_sub_matrix = this->meta_data_set_ptr
                                                      ->get_element(GLOBAL_META, "begin_col_index", sub_matrix_id)
                                                      ->get_metadata_arr()
                                                      ->read_integer_from_arr(0);
    shared_ptr<math_expr_token> col_and_val_read_index_token_ptr(new math_expr_token(BMT_nz_id));
    vector<shared_ptr<basic_token>> col_indices_read_token_vec;
    vector<shared_ptr<basic_token>> vals_read_token_vec;

    data_type vec_result = data_type(data_type_of_result + (unsigned int)log2(this->sparse_coarsen_factor));
    data_type vec_col = data_type(nz_data_type_compress + (unsigned int)log2(this->sparse_coarsen_factor));
    data_type vec_dense = data_type(data_type_of_result + (unsigned int)log2(this->coarsen_factor));


    if (this->need_warp_reduction == false)
    {
        shared_ptr<var_name_token> input_col = code_generator_ptr->generate_global_var(vec_col, "input_col", NULL, 1);
        shared_ptr<var_name_token> input_val = code_generator_ptr->generate_global_var(vec_result, "input_val", NULL, 1);
    }else
    {
        shared_ptr<var_name_token> input_col = code_generator_ptr->generate_global_var(vec_col, "input_col", NULL, 1);
        shared_ptr<var_name_token> input_val = code_generator_ptr->generate_global_var(vec_result, "input_val", NULL, 1);

    }
    shared_ptr<var_name_token> buffer = code_generator_ptr->generate_global_var(vec_dense, "buffer", NULL, 1);


    if (code_generator_ptr->get_interleave_storage() == true)
    {
        assert(this->sparse_coarsen_factor == 1);
        col_indices_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_col_indices_after_interlance_storage", col_and_val_read_index_token_ptr, false);
        vals_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_vals_after_interlance_storage", col_and_val_read_index_token_ptr, false);
        shared_ptr<basic_token> bmt_size;
        if (this->meta_data_set_ptr->is_exist(WARP_META, "first_nz_indices", sub_matrix_id) == true)
        {
            shared_ptr<math_expr_token> warp_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(WARP_META)->run()));
            shared_ptr<math_expr_token> next_warp_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(WARP_META)->run() + "+1"));
            bmt_size = code_generator_ptr->generate_fused_memory_access(WARP_META, "BMT_size_of_each_blk", warp_id);
            shared_ptr<var_name_token> first_bmt_id = code_generator_ptr->generate_fused_memory_access_with_relative(WARP_META, "first_BMT_indices", warp_id);
            shared_ptr<var_name_token> first_bmt_id_next = code_generator_ptr->generate_fused_memory_access_with_relative(WARP_META, "first_BMT_indices", next_warp_id);
            shared_ptr<var_name_token> first_nz_indices_BMW = code_generator_ptr->generate_fused_memory_access_with_relative(WARP_META, "first_nz_indices", warp_id);
            string begin_math_expr = first_nz_indices_BMW->run() + " + " + cur_BMT_id_token->run() + " - " + first_bmt_id->run();
            string end_math_expr = begin_math_expr + " + " + bmt_size->run() + " * (" + first_bmt_id_next->run() + " - " + first_bmt_id->run() + ")";
            for_begin_var = make_shared<math_expr_token>(begin_math_expr);
            for_end_var = make_shared<math_expr_token>(end_math_expr);
            nz_step = first_bmt_id_next->run() + " - " + first_bmt_id->run();            
        }
        else if (this->meta_data_set_ptr->is_exist(TBLOCK_META, "first_nz_indices", sub_matrix_id) == true)
        {
            shared_ptr<math_expr_token> block_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(TBLOCK_META)->run()));
            shared_ptr<math_expr_token> next_block_id(new math_expr_token(code_generator_ptr->code_of_data_block_id_distributed_to_spec_paral(TBLOCK_META)->run() + "+1"));
            bmt_size = code_generator_ptr->generate_fused_memory_access(TBLOCK_META, "BMT_size_of_each_blk", block_id);
            shared_ptr<var_name_token> first_bmt_id = code_generator_ptr->generate_fused_memory_access_with_relative(TBLOCK_META, "first_BMT_indices", block_id);
            shared_ptr<var_name_token> first_bmt_id_next = code_generator_ptr->generate_fused_memory_access_with_relative(TBLOCK_META, "first_BMT_indices", next_block_id);
            shared_ptr<var_name_token> first_nz_indices_BMTB = code_generator_ptr->generate_fused_memory_access_with_relative(TBLOCK_META, "first_nz_indices", block_id);
            string begin_math_expr = first_nz_indices_BMTB->run() + " + " + cur_BMT_id_token->run() + " - " + first_bmt_id->run();
            string end_math_expr = begin_math_expr + " + " + bmt_size->run() + " * (" + first_bmt_id_next->run() + " - " + first_bmt_id->run() + ")";
            for_begin_var = make_shared<math_expr_token>(begin_math_expr);
            for_end_var = make_shared<math_expr_token>(end_math_expr);
            nz_step = first_bmt_id_next->run() + " - " + first_bmt_id->run();
        }
        else
        {
            shared_ptr<math_expr_token> zero_id(new math_expr_token("0"));
            unsigned int BMT_num = this->meta_data_set_ptr->get_element(THREAD_META, "first_nz_indices", sub_matrix_id)->get_metadata_arr()->get_len() - 1;
            bmt_size = code_generator_ptr->generate_fused_memory_access(GLOBAL_META, "BMT_size_of_each_blk", zero_id);
            for_begin_var = make_shared<math_expr_token>(cur_BMT_id_token->run());
            for_end_var = make_shared<math_expr_token>(cur_BMT_id_token->run() + " + " + bmt_size->run() + " * " + to_string(BMT_num));
            nz_step = to_string(BMT_num);
        }
    }
    else
    {

        col_indices_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_col_indices", col_and_val_read_index_token_ptr, false);
        vals_read_token_vec = code_generator_ptr->generate_unfused_memory_access(GLOBAL_META, "nz_vals", col_and_val_read_index_token_ptr, false);

        for_begin_var = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "first_nz_indices", cur_BMT_id_token);
        for_end_var = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "first_nz_indices", next_BMT_id_token);
        nz_step = to_string(this->sparse_coarsen_factor);


    }

    string col_indices_read_name = dynamic_pointer_cast<var_init_token>(col_indices_read_token_vec[0])->get_inited_var_name();
    string var_read_name = dynamic_pointer_cast<var_init_token>(vals_read_token_vec[0])->get_inited_var_name();

    
    shared_ptr<math_expr_token> empty_flag_access(new math_expr_token(cur_BMT_id_token->run() + " / " + to_string((int)get_config()["VECTOR_WIDTH"].as_float())));
    shared_ptr<basic_token> seg_ptr = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "segment_ptr", cur_BMT_id_token);
    shared_ptr<basic_token> seg_flag = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "segment_empty_flag", empty_flag_access);
    shared_ptr<basic_token> thread_bit_map = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "thread_bit_map", cur_BMT_id_token);
    shared_ptr<basic_token> thread_row_indices =  code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "first_row_indices", cur_BMT_id_token);

    string dense_ptr = dense_matrix_ptr->run();



    if (this->need_warp_reduction == true)
    {

        string vec_acc = "float";
        if(data_type_of_result == HALF)
        {
            vec_acc = "half";
        }

        return_code_str << "first_row_thread = " << thread_row_indices->run() << ";" << endl;
        return_code_str << "tmp_row = first_row_thread;" << endl;
        return_code_str << "unsigned int count = 0;" << endl;

        return_code_str << "for(int i = 0; i < " << to_string(this->coarsen_factor) << "; i++)" << endl;
        return_code_str << "{" << endl;
        return_code_str << "temp_result[i] = 0;" << endl;
        return_code_str << result_register_name_1 << "[i] = 0;" << endl;
        return_code_str << "}" << endl;
        return_code_str << "prev_flag = false;" << endl;


        return_code_str << "if(" << dense_ptr << " + " << to_string(this->coarsen_factor) << " <= K)" << endl;
        return_code_str << "{" << endl;


        return_code_str << "for (" << code_of_data_type(nz_data_type) << " " << BMT_nz_id << " = ";
        return_code_str << for_begin_var->run() << "; " << BMT_nz_id << " < " << for_end_var->run() << "; ";
        return_code_str << BMT_nz_id << "+= " << nz_step << ")" << endl;

        return_code_str << "{" << endl;

        if (this->sparse_coarsen_factor == 1)
        {
            for (unsigned int i = 0; i < col_indices_read_token_vec.size(); i++)
            {
                return_code_str << col_indices_read_token_vec[i]->run() << endl;
            }

            for (unsigned int i = 0; i < vals_read_token_vec.size(); i++)
            {
                return_code_str << vals_read_token_vec[i]->run() << endl;
            }

            return_code_str << "input_col[0] = " << col_indices_read_name << ";" << endl;

            return_code_str << "input_val[0] = " << var_read_name << ";" << endl;
        }
        else if (this->sparse_coarsen_factor > 1)
        {
            string col_vec_name = get_metadata_item_name(GLOBAL_META, "nz_col_indices", code_generator_ptr->get_sub_matrix_id());
            string val_vec_name = get_metadata_item_name(GLOBAL_META, "nz_vals", code_generator_ptr->get_sub_matrix_id());  
            return_code_str  << "input_col[0] = Load((" << code_of_data_type(vec_col) << "*)(" << col_vec_name << " + " << BMT_nz_id << "));" << endl;
            return_code_str  << "input_val[0] = Load((" << code_of_data_type(vec_result) << "*)(" << val_vec_name << " + " << BMT_nz_id << "));" << endl;
        }

        if (this->coarsen_factor > 1)
        {

            return_code_str << "for(int c = 0; c < " << to_string(this->sparse_coarsen_factor) << "; c++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "buffer[0] = Load((" << code_of_data_type(vec_dense) << "*)(" << "x_arr" << " + " << dense_ptr <<" + (" << "((" << code_of_data_type(nz_data_type_compress) << " *)input_col)[c]" << " + " << first_col_index_of_sub_matrix << ") * K));"<< endl;

            
        }
        else
        {

            return_code_str << "for(int c = 0; c < " << to_string(this->sparse_coarsen_factor) << "; c++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "buffer[0] = Load((" << code_of_data_type(vec_dense) << "*)(" << "x_arr" << " + " << dense_ptr <<" + (" << "((" << code_of_data_type(nz_data_type_compress) << " *)input_col)[c]" << " + " << first_col_index_of_sub_matrix << ") * K));"<< endl;
        }

        return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
        return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;


        return_code_str << "if(cur_flag == true && (BMT_nz_id + c) !=" << for_begin_var->run() << "){" << endl;
        return_code_str << "count += 1;" << endl;

        return_code_str << "if(prev_flag == true){" << endl;

        vector<shared_ptr<basic_token>> first_row_indices_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
        string row_name = dynamic_pointer_cast<var_init_token>(first_row_indices_of_thread[0])->get_inited_var_name();
        for (int i = 0; i < first_row_indices_of_thread.size(); i++)
        {
            return_code_str << first_row_indices_of_thread[i]->run() << endl;
        }
        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr + r], temp_result[r]);" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr], temp_result[0]);" << endl;
        }

        return_code_str << "}" << endl;
        return_code_str << "else{" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << result_register_1->run() << "[0] = " << "temp_result[0];" << endl;
        }

        return_code_str << "}" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for (unsigned int p = 0; p <" << to_string(this->coarsen_factor) << "; p++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "temp_result[p] = 0;" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << "temp_result[0] = 0;" << endl;
        }

        return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << "){" << endl;
        return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;
        shared_ptr<math_expr_token> empty_id(new math_expr_token("empty_id"));
        vector<shared_ptr<basic_token>> empty_row = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "segment_empty_row_indices", empty_id, false, "");
        string empty_row_name = dynamic_pointer_cast<var_init_token>(empty_row[0])->get_inited_var_name();

        for (int i = 0; i < empty_row.size(); i++)
        {
            return_code_str << empty_row[i]->run() << endl;
        }

        return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";"<< endl;

        return_code_str << "}" << endl;
        return_code_str << "else{" << endl;

        return_code_str << "tmp_row = tmp_row + 1;" << endl; 

        return_code_str << "}" << endl;

        return_code_str << "}" << endl;
        return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
            return_code_str << "{" << endl;

            return_code_str << "temp_result[r] += ((" << code_of_data_type(data_type_of_result) << " *)input_val)[c]" << " * "
                            << "(("<< code_of_data_type(data_type_of_result) << "* )buffer)[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << "temp_result[0] += ((" << code_of_data_type(data_type_of_result) << " *)input_val)[c]" << " * "
                            << "buffer[0];" << endl;
        }

        return_code_str << "}" << endl;

        return_code_str << "}" << endl;

        return_code_str << "if(prev_flag == false){" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r < " << to_string(this->coarsen_factor) << "; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << result_register_1->run() << "[0] = " << "temp_result[0];" << endl;
        }


        return_code_str << "}" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << result_register_2->run() << "[r] = " << "temp_result[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << result_register_2->run() << "[0] = " << "temp_result[0];" << endl;
        }

        return_code_str << "}" << endl;
        return_code_str << "else{" << endl;

        return_code_str << "if(" << dense_ptr << " >= K)"<< endl;
        return_code_str << "return;" << endl;

        return_code_str << "unsigned int size_of_buffer = K - " << dense_ptr << ";" << endl;

        // for循环
        return_code_str << "for (" << code_of_data_type(nz_data_type) << " " << BMT_nz_id << " = ";
        return_code_str << for_begin_var->run() << "; " << BMT_nz_id << " < " << for_end_var->run() << "; ";
        return_code_str << BMT_nz_id << "+= " << nz_step << ")" << endl;

        return_code_str << "{" << endl;

        string col_vec_name = get_metadata_item_name(GLOBAL_META, "nz_col_indices", code_generator_ptr->get_sub_matrix_id());
        string val_vec_name = get_metadata_item_name(GLOBAL_META, "nz_vals", code_generator_ptr->get_sub_matrix_id());
        return_code_str << "input_col[0] = Load((" << code_of_data_type(vec_col) << "*)(" << col_vec_name << " + " << BMT_nz_id << "));" << endl;
        return_code_str << "input_val[0] = Load((" << code_of_data_type(vec_result) << "*)(" << val_vec_name << " + " << BMT_nz_id << "));" << endl;

        if (this->coarsen_factor > 1)
        {

            return_code_str << "for(int c = 0; c < " << to_string(this->sparse_coarsen_factor) << "; c++)" << endl;
            return_code_str << "{" << endl;

            return_code_str << "for(unsigned int q = 0; q < size_of_buffer; q++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "(("<< code_of_data_type(data_type_of_result) << "* )buffer)[q]"
                            << " = ";
            if (first_col_index_of_sub_matrix != 0)
            {
                return_code_str << "x_arr[" << dense_ptr << " + (" << "((" << code_of_data_type(nz_data_type_compress) << " *)input_col)[c]" << " + " << first_col_index_of_sub_matrix << ") * K + q];" << endl;
            }
            else
            {
                return_code_str << "x_arr[" << dense_ptr << " + " << "((" << code_of_data_type(nz_data_type_compress) << " *)input_col)[c]" << " * K + q];" << endl;
            }
            return_code_str << "}" << endl;

        }
        else
        {

           return_code_str << "for(int c = 0; c < " << to_string(this->sparse_coarsen_factor) << "; c++)" << endl;
            return_code_str << "{" << endl;

            return_code_str << "buffer[0] = ";
            if (first_col_index_of_sub_matrix != 0)
            {
                return_code_str << "x_arr[" << dense_ptr << " + ((" << code_of_data_type(nz_data_type_compress) << " *)input_col)[c]" << " + " << first_col_index_of_sub_matrix << ") * K];" << endl;
            }
            else
            {
                return_code_str << "x_arr[" << dense_ptr << " + ((" << code_of_data_type(nz_data_type_compress) << " *)input_col)[c]" << " * K];" << endl;
            }
        }

        return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
        return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;


        return_code_str << "if(cur_flag == true && (BMT_nz_id + c) !=" << for_begin_var->run() << "){" << endl;
        return_code_str << "count += 1;" << endl;


        return_code_str << "if(prev_flag == true){" << endl;

        for (int i = 0; i < first_row_indices_of_thread.size(); i++)
        {
            return_code_str << first_row_indices_of_thread[i]->run() << endl;
        }

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr + r], temp_result[r]);" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr], temp_result[0]);" << endl;
        }

        return_code_str << "}" << endl;
        return_code_str << "else{" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << result_register_1->run() << "[0] = " << "temp_result[0];" << endl;
        }

        return_code_str << "}" << endl;
        if (this->coarsen_factor > 1)
        {
            return_code_str << "for (unsigned int p = 0; p < size_of_buffer; p++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "temp_result[p] = 0;" << endl;
            return_code_str << "}" << endl;
        }else
        {
            return_code_str << "temp_result[0] = 0;" << endl;
        }

        return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << "){" << endl;
        return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;

        for (int i = 0; i < empty_row.size(); i++)
        {
            return_code_str << empty_row[i]->run() << endl;
        }

        return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";" << endl;

        return_code_str << "}" << endl;
        return_code_str << "else{" << endl;

        return_code_str << "tmp_row = tmp_row + 1;" << endl; 

        return_code_str << "}" << endl;

        return_code_str << "}" << endl;
        return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
            return_code_str << "{" << endl;

            return_code_str << "temp_result[r] += ((" << code_of_data_type(data_type_of_result) << " *)input_val)[c]" << " * "
                            << "(("<< code_of_data_type(data_type_of_result) << "* )buffer)[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        { 
            return_code_str << "temp_result[0] += ((" << code_of_data_type(data_type_of_result) << " *)input_val)[c]" << " * "
                            << "buffer[0];" << endl;
        }

        return_code_str << "}" << endl;
        return_code_str << "}" << endl;

        return_code_str << "if(prev_flag == false){" << endl;
        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << result_register_1->run() << "[r] = " << "temp_result[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << result_register_1->run() << "[0] = " << "temp_result[0];" << endl;
        }

        return_code_str << "}" << endl;

        if (this->coarsen_factor > 1)
        {
            return_code_str << "for(unsigned int r = 0; r < size_of_buffer; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << result_register_2->run() << "[r] = " << "temp_result[r];" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << result_register_2->run() << "[0] = " << "temp_result[0];" << endl;
        }

        return_code_str << "}" << endl;
        shared_ptr<var_name_token> segment_offset_fuse = code_generator_ptr->generate_fused_memory_access_with_relative(THREAD_META, "segment_offset", cur_BMT_id_token);
        return_code_str << segment_offset->run() << " = " << segment_offset_fuse->run() << ";" << endl;
    }
    else
    {
        if (this->coarsen_factor > 1)
        {
            return_code_str << "for (unsigned int p = 0; p <" << to_string(this->coarsen_factor) << "; p++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << result_register_name_1 << "[p] = 0;" << endl;
            return_code_str << result_register_name_2 << "[p] = 0;" << endl;
            return_code_str << "temp_result[p] = 0;" << endl;
            return_code_str << "}" << endl;
        }
        else
        {
            return_code_str << result_register_name_1 << "[0] = 0;" << endl;
            return_code_str << result_register_name_2 << "[0] = 0;" << endl;
            return_code_str << "temp_result[0] = 0;" << endl;        
        }

        return_code_str << "prev_flag = false;" << endl;
        return_code_str << "cur_flag = false;" << endl;
        return_code_str << "first_row_thread = " << thread_row_indices->run() << ";" << endl;
        return_code_str << "tmp_row = first_row_thread;" << endl;
        return_code_str << "unsigned int count = 0;" << endl;

        return_code_str << "for(int i = 0; i < " << to_string(this->coarsen_factor) << "; i++)" << endl;
        return_code_str << "{" << endl;
        return_code_str << "temp_result[i] = 0;" << endl;
        return_code_str << result_register_name_1 << "[i] = 0;" << endl;
        return_code_str << "}" << endl;

        // for循环
        return_code_str << "for (" << code_of_data_type(nz_data_type) << " " << BMT_nz_id << " = ";
        return_code_str << for_begin_var->run() << "; " << BMT_nz_id << " < " << for_end_var->run() << "; ";
        return_code_str << BMT_nz_id << "+= " << nz_step << ")" << endl;

        return_code_str << "{" << endl;

        if (this->sparse_coarsen_factor == 1)
        {
            for (unsigned int i = 0; i < col_indices_read_token_vec.size(); i++)
            {
                return_code_str << col_indices_read_token_vec[i]->run() << endl;
            }

            for (unsigned int i = 0; i < vals_read_token_vec.size(); i++)
            {
                return_code_str << vals_read_token_vec[i]->run() << endl;
            }

            return_code_str << "input_col[0] = " << col_indices_read_name << ";" << endl;

            return_code_str << "input_val[0] = " << var_read_name << ";" << endl;
        }
        else
        {

            string col_vec_name = get_metadata_item_name(GLOBAL_META, "nz_col_indices", code_generator_ptr->get_sub_matrix_id());
            string val_vec_name = get_metadata_item_name(GLOBAL_META, "nz_vals", code_generator_ptr->get_sub_matrix_id());

            return_code_str << "input_col[0] = Load((" << code_of_data_type(vec_col) << "*)(" << col_vec_name << " + " << BMT_nz_id << "));" << endl;
            return_code_str << "input_val[0] = Load((" << code_of_data_type(vec_result) << "*)(" << val_vec_name << " + " << BMT_nz_id << "));" << endl;
        }


        return_code_str << "for(int j = 0; j < "<< to_string(this->sparse_coarsen_factor) <<" ; j++)" << endl;
        return_code_str << "{" << endl;
        if (this->coarsen_factor > 1)
        {

            return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
            return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;

            return_code_str << "if(cur_flag == true && (BMT_nz_id + j) !=" << for_begin_var->run() << "){" << endl;
            return_code_str << "count += 1;" << endl;

           

            vector<shared_ptr<basic_token>> cur_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
            string row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
            for (int i = 0; i < cur_row_of_thread.size(); i++)
            {
                return_code_str << cur_row_of_thread[i]->run() << endl;
            }

            return_code_str << "for (int r = 0; r < " << this->coarsen_factor << "; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr + r], temp_result[r]);" << endl;
            return_code_str << "}" << endl;

            return_code_str << "for (unsigned int p = 0; p <" << to_string(this->coarsen_factor) << "; p++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "temp_result[p] = 0;" << endl;
            return_code_str << "}" << endl;

            return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << ")"
                            << "{" << endl;
            return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;
            shared_ptr<math_expr_token> empty_id(new math_expr_token("empty_id"));
            vector<shared_ptr<basic_token>> empty_row = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "segment_empty_row_indices", empty_id, false, "");
            string empty_row_name = dynamic_pointer_cast<var_init_token>(empty_row[0])->get_inited_var_name();

            for (int i = 0; i < empty_row.size(); i++)
            {
                return_code_str << empty_row[i]->run() << endl;
            }

            return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";" << endl;

            return_code_str << "}" << endl;
            return_code_str << "else{" << endl;

            return_code_str << "tmp_row = tmp_row + 1;" << endl;

            return_code_str << "}" << endl;

            return_code_str << "}" << endl;
            return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;


            return_code_str << "buffer[0] = Load((" << code_of_data_type(vec_dense) << "*)(" << "x_arr" << " + " << dense_ptr << " + (" << "((" << code_of_data_type(nz_data_type_compress) << " *)input_col)[j]" << " + " << first_col_index_of_sub_matrix << ") * K));" << endl;
            
            return_code_str << "for(unsigned int r = 0; r <" << to_string(this->coarsen_factor) << "; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "temp_result[r] +=  (("<< code_of_data_type(data_type_of_result) << "* )input_val)[j] * "
                            << "(("<< code_of_data_type(data_type_of_result) << "* )buffer)[r];" << endl;
            return_code_str << "}" << endl;

            return_code_str << "}" << endl;
            return_code_str << "}" << endl;
            vector<shared_ptr<basic_token>> last_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
            string last_row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
            for (int i = 0; i < cur_row_of_thread.size(); i++)
            {
                return_code_str << cur_row_of_thread[i]->run() << endl;
            }

            return_code_str << "for (int r = 0; r < " << this->coarsen_factor << "; r++)" << endl;
            return_code_str << "{" << endl;
            return_code_str << "atomicAdd(&y_arr[" << last_row_name << " * K + dense_matrix_ptr + r], temp_result[r]);" << endl;

            return_code_str << "}" << endl;

        }
        else
        {

           return_code_str << "cur_flag = " << thread_bit_map->run() << " & 1;" << endl;
            return_code_str << thread_bit_map->run() << " = " << thread_bit_map->run() << " >> 1;" << endl;

            return_code_str << "if(cur_flag == true && (BMT_nz_id + j) !=" << for_begin_var->run() << "){" << endl;
           
            return_code_str << "count += 1;" << endl;

            vector<shared_ptr<basic_token>> cur_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
            string row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
            for (int i = 0; i < cur_row_of_thread.size(); i++)
            {
                return_code_str << cur_row_of_thread[i]->run() << endl;
            }


            return_code_str << "atomicAdd(&y_arr[" << row_name << " * K + dense_matrix_ptr], temp_result[0]);" << endl;
            return_code_str << "temp_result[0] = 0;" << endl;

            return_code_str << "if((" << seg_flag->run() << " >> lane_id) & 1 == true" << "){" << endl;
            return_code_str << "unsigned long empty_id = " << seg_ptr->run() << " + count;" << endl;
            shared_ptr<math_expr_token> empty_id(new math_expr_token("empty_id"));
            vector<shared_ptr<basic_token>> empty_row = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "segment_empty_row_indices", empty_id, false, "");
            string empty_row_name = dynamic_pointer_cast<var_init_token>(empty_row[0])->get_inited_var_name();

            for (int i = 0; i < empty_row.size(); i++)
            {
                return_code_str << empty_row[i]->run() << endl;
            }

            return_code_str << "tmp_row = " << thread_row_indices->run() << " + " << empty_row_name << ";" << endl;

            return_code_str << "}" << endl;
            return_code_str << "else{" << endl;

            return_code_str << "tmp_row = tmp_row + 1;" << endl;

            return_code_str << "}" << endl;

            return_code_str << "}" << endl;
            return_code_str << "prev_flag = prev_flag | cur_flag;" << endl;

            return_code_str << "buffer[0] = " << endl;
            if (first_col_index_of_sub_matrix != 0)
            {
                return_code_str << "__ldg(&(x_arr[dense_matrix_ptr + (((" << code_of_data_type(nz_data_type_compress) << "* )input_col)[j] + " << first_col_index_of_sub_matrix << ") * K]));" << endl;
            }
            else
            {
                return_code_str << "__ldg(&(x_arr[dense_matrix_ptr + ((" << code_of_data_type(nz_data_type_compress) << "* )input_col)[j] * K]));" << endl;
            }

            return_code_str << "temp_result[0] +=  (("<< code_of_data_type(data_type_of_result) << "* )input_val)[j] * "
                            << "buffer[0];" << endl;

            return_code_str << "}" << endl;
            return_code_str << "}" << endl;

            vector<shared_ptr<basic_token>> last_row_of_thread = code_generator_ptr->generate_unfused_memory_access(THREAD_META, "first_row_indices", cur_BMT_id_token, true, "tmp_row");
            string last_row_name = dynamic_pointer_cast<var_init_token>(cur_row_of_thread[0])->get_inited_var_name();
            for (int i = 0; i < cur_row_of_thread.size(); i++)
            {
                return_code_str << cur_row_of_thread[i]->run() << endl;
            }
            return_code_str << "atomicAdd(&y_arr[" << last_row_name << " * K + dense_matrix_ptr], temp_result[0]);" << endl;


        }
    }

    return return_code_str.str();
}

shared_ptr<basic_IO_of_reduction>thread_bit_map_reduce_to_two_register_token::get_output_IO()
{
    return this->output_IO;
}

