#include "StateManager.h"


void StateManager::run_cmd(bool isfirst)
{
	using namespace Eigen;
	using namespace std;

	after_load();
	parafun_solver.reset(new Parafun(scaf_data));
	//printf("%s\n", s_.model_file.c_str());
	if (parafun_solver->Cur_pe < parafun_solver->packing_efficiency)
	{
		out_related.typestr = "typeD";
		//ofs << "typeD init_pe " << parafun_solver->Cur_pe << " is lower than the pe_bound " << parafun_solver->packing_efficiency << " ;so return" << endl;
		return;
	}

	double last_mesh_energy = parafun_solver->best_cache.distortion;
	double bigger_factor = scaf_data.bigger_factor_init;

	Eigen::MatrixXd uv_origin;
	long t0,t1;
	if (isfirst)
	{
		t0 = clock();
		uv_origin = scaf_data.w_uv;
	}


	double conv_rate_flag = 1e-3;
	parafun_solver->Plus_area_term_search = false;

	double last_conv_e = last_mesh_energy;
	bool isnot_convged = true;
	double conv_rate;
	while (last_mesh_energy > src_distortion&&isnot_convged)
	{
		iter_count = 0;
		scaf_data.rect_frame_V.resize(0, 0);

		std::cout << "Resize Scaffold..." << std::endl;
		while (iter_count < parafun_solver->MAX_ITER_NUM)
		{
			iter_count++;
			iter_num_sum++;

			printf("Iter %3d: ", iter_num_sum);
			perform_one_iteration(bigger_factor, last_mesh_energy, conv_rate);
			notify_iteration();
			double expect_conv_rate = (last_mesh_energy - src_distortion) / last_mesh_energy;

			int iterations_needed = (int)(expect_conv_rate / conv_rate);

			//printf("conv_rate = %.9f; expect_conv_rate = %.9f; N %d\n", conv_rate, expect_conv_rate, iterations_needed);

			conv_rate_flag = 1e-2;
			if (iterations_needed < 10)
				conv_rate_flag = 1e-5;
			else
				conv_rate_flag = 1e-2;

			if (conv_rate < conv_rate_flag || last_mesh_energy < src_distortion)
			{
				double conv_conv_rate = (last_conv_e - last_mesh_energy) / last_conv_e;
				last_conv_e = last_mesh_energy;
				if (conv_conv_rate < 1e-6)
					isnot_convged = false;
				//printf("Cur_pe: %f; Cur_dis: %f; iter_num: %d\n", parafun_solver->Cur_pe, last_mesh_energy, iter_count);
				break;
			}
		}
		//if (abs(parafun_solver->Cur_pe - parafun_solver->packing_efficiency) < 1e-5)
		if (parafun_solver->Cur_pe < parafun_solver->packing_efficiency)
			break;
		bigger_factor += 0.005;
	}

	int aug_times_num;
	if (parafun_solver->Cur_pe < parafun_solver->packing_efficiency)
	{
		{
			conv_rate_flag = 1e-5;
			last_mesh_energy = parafun_solver->best_cache.distortion;
			scaf_data.w_uv = parafun_solver->best_cache.w_uv;
			parafun_solver->Plus_area_term_search = true;
			std::cout << "PE lower than bound, rebegin the last factor with area constraint" << std::endl;
			while (last_mesh_energy > src_distortion)
			{
				iter_count = 0;
				scaf_data.rect_frame_V.resize(0, 0);
				std::cout << "Resize Scaffold..." << std::endl;
				while (iter_count < parafun_solver->MAX_ITER_NUM)
				{
					iter_count++;
					iter_num_sum++;
					printf("Iter %3d: ", iter_num_sum);
					perform_one_iteration(bigger_factor, last_mesh_energy, conv_rate);
					notify_iteration();

					if (conv_rate < conv_rate_flag || last_mesh_energy < src_distortion)
					{
						//printf("Cur_pe: %f; Cur_dis: %f; iter_num: %d\n", parafun_solver->Cur_pe, last_mesh_energy, iter_count);
						break;
					}
				}
				if (abs(parafun_solver->Cur_pe - parafun_solver->packing_efficiency) < 1e-6)
				{
					break;
				}
				bigger_factor += 0.005;
			}
		}
		out_related.cur_dis = last_mesh_energy / 4.0 + 1.0;
		aug_times_num = round((bigger_factor - scaf_data.bigger_factor_init) / 0.005);

		if (last_mesh_energy <= src_distortion)
		{
			out_related.src_dis = src_distortion / 4.0 + 1.0;
			//printf("Successful energy %f has been first below the src_energy %f , and pe %f is reach than pe_bound %f", last_mesh_energy, src_distortion, parafun_solver->Cur_pe, parafun_solver->packing_efficiency);
			//ofs << "typeA Successful energy " << last_mesh_energy << " has been first below the src_energy " << src_distortion << " , and pe " << parafun_solver->Cur_pe << " is higher than the pe_bound " << parafun_solver->packing_efficiency;
			out_related.typestr = "typeA";

		}
		else
		{
			if (isfirst&& scaf_data.bigger_factor_init > 1.005 && (aug_times_num == 0 || last_mesh_energy > 10.0*src_distortion))
			{
				printf("enter run__second\n");
				scaf_data.bigger_factor_init = 1.005;
				scaf_data.w_uv = uv_origin;
				run_cmd(false);
				t1 = clock();
				out_related.time_sum = (t1-t0)/1000.0;
				return;
			}
			else
			{
				out_related.src_dis = src_distortion / 4.0 + 1.0;
				//d>src&&pe=Bound
				//printf("Fail case final energy %f higher than the src_energy %f , and pe %f is reach than pe_bound %f", last_mesh_energy, src_distortion, parafun_solver->Cur_pe, parafun_solver->packing_efficiency);
				//ofs << "typeB Fail case final energy " << last_mesh_energy << " has been first below the src_energy " << src_distortion << " , and pe " << parafun_solver->Cur_pe << " is reach the pe_bound " << parafun_solver->packing_efficiency;
				out_related.typestr = "typeB";

			}

		}
		//printf("Fail case: Record Cur_pe: %f; Cur_dis: %f\n", parafun_solver->best_cache.pe, parafun_solver->best_cache.distortion);	
	}
	else
	{
		out_related.cur_dis = last_mesh_energy / 4.0 + 1.0;
		out_related.src_dis = src_distortion / 4.0 + 1.0;
		aug_times_num = round((bigger_factor - scaf_data.bigger_factor_init) / 0.005);
		//d<src&pe>Bound
		//printf("Successful energy %f has been first below the src_energy %f , and pe %f is higher than pe_bound %f", last_mesh_energy, src_distortion, parafun_solver->Cur_pe, parafun_solver->packing_efficiency);
		//ofs << "typeC Successful energy " << last_mesh_energy << " has been first below the src_energy " << src_distortion << " , and pe " << parafun_solver->Cur_pe << " is higher than pe_bound " << parafun_solver->packing_efficiency;
		out_related.typestr = "typeA";

	}
	//A>B>D
	//printf("init factor: %f; the end factor: %f; augment times number: %d; sum_iteration_times: %d;\n", scaf_data.bigger_factor_init, bigger_factor, aug_times_num, iter_num_sum);
	if (isfirst)
	{
		t1 = clock();
		out_related.time_sum = (t1 - t0) / 1000.0;
	}

}

void StateManager::run_cmd_2bound(bool isfirst)
{
	using namespace Eigen;
	using namespace std;

	after_load();
	parafun_solver.reset(new Parafun(scaf_data));
	//printf("%s\n", s_.model_file.c_str());
	if (parafun_solver->Cur_pe < parafun_solver->packing_efficiency)
	{
		out_related.typestr = "typeD";
		//ofs << "typeD init_pe " << parafun_solver->Cur_pe << " is lower than the pe_bound " << parafun_solver->packing_efficiency<<" ;so return" << endl;
		return;
	}

	double last_mesh_energy = parafun_solver->best_cache.distortion;
	double bigger_factor = scaf_data.bigger_factor_init;

	Eigen::MatrixXd uv_origin;
	long t0, t1;
	if (isfirst)
	{
		t0 = clock();
		uv_origin = scaf_data.w_uv;
	}
	double last_conv_e = last_mesh_energy;

	double conv_rate_flag = 1e-3;

	bool isnot_convged = true;
	double conv_rate;
	while (isnot_convged)
	{
		parafun_solver->Plus_area_term_search = false;
		iter_count = 0;
		scaf_data.rect_frame_V.resize(0, 0);

		std::cout << "bigger_factor =========================== " << bigger_factor << std::endl;
		while (iter_count < parafun_solver->MAX_ITER_NUM)
		{
			iter_count++;
			iter_num_sum++;

			perform_one_iteration(bigger_factor, last_mesh_energy, conv_rate);
			notify_iteration();

			if (conv_rate < conv_rate_flag)
			{
				double conv_conv_rate = (last_conv_e - last_mesh_energy) / last_conv_e;
				last_conv_e = last_mesh_energy;
				if (conv_conv_rate < 1e-6)
					isnot_convged = false;
				//printf("Cur_pe: %f; Cur_dis: %f; iter_num: %d\n", parafun_solver->Cur_pe, last_mesh_energy, iter_count);

				break;
			}
		}
		if (parafun_solver->Cur_pe < parafun_solver->packing_efficiency)
		{
			break;
		}
		bigger_factor += 0.005;
	}

	int aug_times_num;
	if (isnot_convged)
	{
		aug_times_num = round((bigger_factor - scaf_data.bigger_factor_init) / 0.005);

		if (isfirst&& scaf_data.bigger_factor_init > 1.005&&((aug_times_num == 0 &&parafun_solver->best_cache.distortion> src_distortion)|| parafun_solver->best_cache.distortion >10.*src_distortion))
		{
			printf("enter run__second\n");
			scaf_data.bigger_factor_init = 1.005;
			scaf_data.w_uv = uv_origin;
			run_cmd_2bound(false);
			t1 = clock();
			out_related.time_sum = (t1 - t0) / 1000.0;
			return;
		}
		else
		{
			last_mesh_energy = parafun_solver->best_cache.distortion;
			scaf_data.w_uv = parafun_solver->best_cache.w_uv;
			parafun_solver->Plus_area_term_search = true;
			std::cout << " ========================================================================= " << std::endl;
			conv_rate_flag = 1e-5;
			while (isnot_convged)
			{
				iter_count = 0;
				scaf_data.rect_frame_V.resize(0, 0);

				std::cout << "bigger_factor =========================== " << bigger_factor << std::endl;
				while (iter_count < parafun_solver->MAX_ITER_NUM)
				{
					iter_count++;
					iter_num_sum++;
					perform_one_iteration(bigger_factor, last_mesh_energy, conv_rate);
					notify_iteration();

					if (conv_rate < conv_rate_flag)
					{
						//printf("Cur_pe: %f; Cur_dis: %f; iter_num: %d\n", parafun_solver->Cur_pe, last_mesh_energy, iter_count);
						break;
					}
				}
				if (abs(parafun_solver->Cur_pe - parafun_solver->packing_efficiency) < 1e-6)
				{
					break;
				}
				bigger_factor += 0.005;
			}
		}
	}

	out_related.cur_dis = last_mesh_energy / 4.0 + 1.0;
	out_related.src_dis = src_distortion / 4.0 + 1.0;
	if (!isnot_convged)
	{
		//printf("typeA energy has come to the lowest point, and pe %f is higher than pe_bound %f", parafun_solver->Cur_pe, parafun_solver->packing_efficiency);
		//ofs << "typeA energy has come to the lowest point " << last_mesh_energy << "  ofcourse lower than src_energy " << src_distortion << "  and final pe " << parafun_solver->Cur_pe << " is still higher than pe_bound " << parafun_solver->packing_efficiency;
		out_related.typestr = "typeC";
	}
	else
	{
		if (last_mesh_energy <= src_distortion)
		{
			//printf("typeB pe reach the pe_bound,and the final energy is %f ,lower than src_energy %f", last_mesh_energy, src_distortion);
			//ofs << "typeB pe reach the pe_bound,and the final energy " << last_mesh_energy << " ,lower than src_energy " << src_distortion << " , and pe " << parafun_solver->Cur_pe << " is reach the pe_bound " << parafun_solver->packing_efficiency;
			out_related.typestr = "typeA";
		}
		else
		{
			//printf("typeC pe reach the pe_bound,but the final energy is %f ,higher than src_energy %f", last_mesh_energy, src_distortion);
			//ofs << "typeC pe reach the pe_bound,but the final energy " << last_mesh_energy << " ,higher than src_energy " << src_distortion << " , and pe " << parafun_solver->Cur_pe << " is reach the pe_bound " << parafun_solver->packing_efficiency;
			out_related.typestr = "typeB";
		}
	}
	//A>B>C>D

	aug_times_num = round((bigger_factor - scaf_data.bigger_factor_init) / 0.005);
	//printf("Init factor: %f; the end factor: %f; augment times number: %d; sum_iteration_times: %d;\n", scaf_data.bigger_factor_init, bigger_factor, aug_times_num, iter_num_sum);
	if (isfirst)
	{
		t1 = clock();
		out_related.time_sum = (t1 - t0) / 1000.0;
	}

}

void StateManager::run(std::string filename, std::string filename_e,double gap, const string& type)
{
	cout << filename << " begin====================" << endl;

	model_file = filename;
	string dis_str = filename_e;
	ifstream ifs(dis_str);
	string line;
	getline(ifs, line);
	src_distortion = atof(line.c_str());
	src_distortion -= 4.0;
	std::cout << "src_distortion: " << src_distortion << endl;

	atlas_init(filename, scaf_data);

	if (scaf_data.separated_V_UV.size() == 1)
		gap = 0.0;
	char gapchar[5];
	sprintf(gapchar, "%.1f", gap);
	out_related.gapstr = gapchar;
	scaf_data.gap_distance = gap;
	scaf_data.is_gap = (gap <= 1e-6 ? false : true);
	iter_num_sum = 0;

	if (strcmp(type.c_str(), "LOWER")==0)
	{
		out_related.outtxtname = "record_lowerthansrc_gap" + out_related.gapstr + ".txt";
		out_related.outobjstr = model_file.substr(0, model_file.find_last_of('/') + 1) + "mesh_out_gap" + out_related.gapstr + ".obj";
		run_cmd();
		update_uv();
		printf("Time: %.3fs\n", out_related.time_sum);
	}
	else if (strcmp(type.c_str(), "TOBOUND")==0)
	{
		out_related.outtxtname = "record_2bound_gap" + out_related.gapstr + ".txt";
		out_related.outobjstr = model_file.substr(0, model_file.find_last_of('/') + 1) + "mesh_out_2b_gap" + out_related.gapstr + ".obj";
		run_cmd_2bound();
		update_uv();
		printf("Time: %.3fs\n", out_related.time_sum);
	}
	else
	{
		cout << "Error:Wrong Type!!!!!!" << endl;
	}
}

void StateManager::run_interface(const Eigen::MatrixXd & v_pos, const Eigen::MatrixXd & uv_v_pos, Eigen::MatrixXi & fv_id, Eigen::MatrixXi & uv_fv_id, double dis_bound, double gap, double peb, const string & type, IterationCallback callback)
{
	iteration_callback = std::move(callback);
	src_distortion = dis_bound;
	src_distortion -= 4.0;
	atlas_init_interface(v_pos, uv_v_pos, fv_id, uv_fv_id);
	if (scaf_data.separated_V_UV.size() == 1) gap = 0.0;
	scaf_data.PE_BOUND = peb;

	char gapchar[5];
	sprintf(gapchar, "%.1f", gap);
	out_related.gapstr = gapchar;
	scaf_data.gap_distance = gap;
	scaf_data.is_gap = (gap <= 1e-6 ? false : true);
	iter_num_sum = 0;

	if (strcmp(type.c_str(), "LOWER") == 0)
	{
		out_related.outtxtname = "record_lowerthansrc_gap" + out_related.gapstr + ".txt";
		out_related.outobjstr = model_file.substr(0, model_file.find_last_of('/') + 1) + "mesh_out_gap" + out_related.gapstr + ".obj";
		run_cmd();
		update_uv();
		printf("Time: %.3fs\n", out_related.time_sum);
	}
	else if (strcmp(type.c_str(), "TOBOUND") == 0)
	{
		out_related.outtxtname = "record_2bound_gap" + out_related.gapstr + ".txt";
		out_related.outobjstr = model_file.substr(0, model_file.find_last_of('/') + 1) + "mesh_out_2b_gap" + out_related.gapstr + ".obj";
		run_cmd_2bound();
		update_uv();
		printf("Time: %.3fs\n", out_related.time_sum);
	}
	else
	{
		cout << "Error:Wrong Type!!!!!!" << endl;
	}

}

void StateManager::notify_iteration()
{
	if (!iteration_callback) return;
	update_uv();
	iteration_callback(scaf_data.UV_V, iter_num_sum);
}

void StateManager::update_uv()
{
	int accu_uv = 0;
	for (int i = 0; i < scaf_data.UV_VI.size(); i++)
	{
		for (int j = 0; j < scaf_data.UV_VI[i].rows(); j++)
		{
			if (scaf_data.UV_VI[i](j) == -1) continue;

			scaf_data.UV_V.row(j) = scaf_data.w_uv.row(scaf_data.UV_VI[i](j) + accu_uv);
		}

		accu_uv += scaf_data.separated_V_UV[i].rows();
	}
}

void StateManager::after_load()
{
	auto &d_ = scaf_data;
	d_.rect_frame_V.resize(0, 0);
	{
		MatrixXd m_uv = d_.w_uv.topRows(d_.mv_num);

		d_.initbox.uv_max = m_uv.colwise().maxCoeff();
		d_.initbox.uv_min = m_uv.colwise().minCoeff();

		if (d_.is_gap)
		{
			d_.initbox.dx = d_.gap_distance;
			d_.initbox.uv_max(0) += d_.initbox.dx;
			d_.initbox.uv_max(1) += d_.initbox.dx;
			d_.initbox.uv_min(0) -= d_.initbox.dx;
			d_.initbox.uv_min(1) -= d_.initbox.dx;
		}
		d_.initbox.uv_mid = (d_.initbox.uv_max + d_.initbox.uv_min) / 2.;
	}

	if (d_.is_gap)
	{
		d_.mesh_improve_inner_init(true, d_.bigger_factor_init);
	}
	else
	{
		d_.mesh_improve(true, d_.bigger_factor_init);
	}
}

void StateManager::atlas_init(std::string filename, ScafData & d_)
{
	using namespace Eigen;
	using namespace std;

	Eigen::MatrixXd corner_normals;
	Eigen::MatrixXi fNormIndices;

	MatrixXd& V = d_.V;
	MatrixXd& UV_V = d_.UV_V;
	MatrixXi& F = d_.F;
	MatrixXi& UV_F = d_.UV_F;

	readobj(filename, V, UV_V, F, UV_F);

	Eigen::SparseMatrix<int> Adj;
	Eigen::MatrixXi V_conn_flag;
	MatrixXi component_vert_sizes;
	adjacency_matrix(UV_F, Adj);
	components(Adj, V_conn_flag, component_vert_sizes);
	//std::cout << "counts:" << component_vert_sizes << std::endl;
	int component_number = component_vert_sizes.size();

	VectorXi component_face_sizes = Eigen::VectorXi::Zero(component_number);
	Eigen::VectorXi F_conn_flag(UV_F.rows());

	for (int i = 0; i < UV_F.rows(); i++)
	{
		int flag = V_conn_flag(UV_F(i, 0));
		F_conn_flag(i) = flag;
		component_face_sizes(flag)++;
	}
	//std::cout << "comp_face_counts:" << component_face_sizes << std::endl;
	std::vector<MatrixXd> separated_V(component_number);
	std::vector<MatrixXi> separated_F_UV(component_number);


	//std::vector<Eigen::MatrixXd> separated_V_UV;
	//std::vector<Eigen::VectorXi> UV_VI;

	auto& separated_V_UV = d_.separated_V_UV;
	auto& UV_VI = d_.UV_VI;

	separated_V_UV.resize(component_number);
	UV_VI.resize(component_number);

	VectorXd chart_factors;
	double total_area = 0.0;
	double total_uv_area = 0.0;
	chart_factors.resize(component_number);
	for (int i = 0; i < component_number; i++)
	{
		Eigen::MatrixXi F_temp(component_face_sizes(i), 3);
		Eigen::MatrixXi F_temp_UV(component_face_sizes(i), 3);

		std::map<int, int> id_uv_mesh;
		int filler = 0;
		for (int j = 0; j < UV_F.rows(); j++)
		{
			if (F_conn_flag(j) == i)
			{
				//F_temp.row(filler) = F.row(j);
				F_temp_UV.row(filler) = UV_F.row(j);
				filler++;

				id_uv_mesh[UV_F(j, 0)] = F(j, 0);
				id_uv_mesh[UV_F(j, 1)] = F(j, 1);
				id_uv_mesh[UV_F(j, 2)] = F(j, 2);
			}
		}

		remove_unreferenced(UV_V, F_temp_UV, separated_V_UV[i], separated_F_UV[i], UV_VI[i]);

		separated_V[i].resize(separated_V_UV[i].rows(), 3);
		for (int j = 0; j < UV_VI[i].rows(); j++)
		{
			if (UV_VI[i](j) == -1) continue;
			separated_V[i].row(UV_VI[i](j)) = V.row(id_uv_mesh.at(j));
		}

		VectorXd M, UV_M;
		doublearea(separated_V[i], separated_F_UV[i], M);
		doublearea(separated_V_UV[i], separated_F_UV[i], UV_M);

		total_area += M.sum();
		total_uv_area += UV_M.sum();
		chart_factors[i] = std::sqrt(UV_M.sum() / M.sum());
		//d_.add_new_chart(separated_V[i], separated_F_UV[i], separated_V_UV[i], separated_F_UV[i]);
	}

	double max_factor = chart_factors.maxCoeff();

	double avg_factor = std::sqrt(total_uv_area / total_area);
	d_.atlas_factor = max_factor / avg_factor;
	//std::cout << "Factor: Max " << chart_factors.maxCoeff() << ", Min " << chart_factors.minCoeff() << ", Avg " << atlas_factor << std::endl;
	for (int i = 0; i < component_number; i++)
	{
		separated_V[i] *= max_factor;
		d_.add_new_chart(separated_V[i], separated_F_UV[i], separated_V_UV[i], separated_F_UV[i]);
	}

	d_.bigger_factor_init = std::max(1.005, 0.9*d_.atlas_factor);
	//cout << "Atlas_factor: " << d_.atlas_factor << endl;
	return;

}

void StateManager::atlas_init_interface(const Eigen::MatrixXd & v_pos, const Eigen::MatrixXd & uv_v_pos, const Eigen::MatrixXi & fv_id,const Eigen::MatrixXi & uv_fv_id)
{
	ScafData& d_ = scaf_data;

	MatrixXd& V = scaf_data.V;
	MatrixXd& UV_V = scaf_data.UV_V;
	MatrixXi& F = scaf_data.F;
	MatrixXi& UV_F = scaf_data.UV_F;

	V = v_pos;
	UV_V = uv_v_pos;
	F = fv_id;
	UV_F = uv_fv_id;

	Eigen::SparseMatrix<int> Adj;
	Eigen::MatrixXi V_conn_flag;
	MatrixXi component_vert_sizes;
	adjacency_matrix(UV_F, Adj);
	components(Adj, V_conn_flag, component_vert_sizes);
	//std::cout << "counts:" << component_vert_sizes << std::endl;
	int component_number = component_vert_sizes.size();

	VectorXi component_face_sizes = Eigen::VectorXi::Zero(component_number);
	Eigen::VectorXi F_conn_flag(UV_F.rows());

	for (int i = 0; i < UV_F.rows(); i++)
	{
		int flag = V_conn_flag(UV_F(i, 0));
		F_conn_flag(i) = flag;
		component_face_sizes(flag)++;
	}
	//std::cout << "comp_face_counts:" << component_face_sizes << std::endl;
	std::vector<MatrixXd> separated_V(component_number);
	std::vector<MatrixXi> separated_F_UV(component_number);


	auto& separated_V_UV = d_.separated_V_UV;
	auto& UV_VI = d_.UV_VI;

	separated_V_UV.resize(component_number);
	UV_VI.resize(component_number);

	VectorXd chart_factors;
	double total_area = 0.0;
	double total_uv_area = 0.0;
	chart_factors.resize(component_number);
	for (int i = 0; i < component_number; i++)
	{
		Eigen::MatrixXi F_temp(component_face_sizes(i), 3);
		Eigen::MatrixXi F_temp_UV(component_face_sizes(i), 3);

		std::map<int, int> id_uv_mesh;
		int filler = 0;
		for (int j = 0; j < UV_F.rows(); j++)
		{
			if (F_conn_flag(j) == i)
			{
				//F_temp.row(filler) = F.row(j);
				F_temp_UV.row(filler) = UV_F.row(j);
				filler++;

				id_uv_mesh[UV_F(j, 0)] = F(j, 0);
				id_uv_mesh[UV_F(j, 1)] = F(j, 1);
				id_uv_mesh[UV_F(j, 2)] = F(j, 2);
			}
		}
		remove_unreferenced(UV_V, F_temp_UV, separated_V_UV[i], separated_F_UV[i], UV_VI[i]);

		separated_V[i].resize(separated_V_UV[i].rows(), 3);
		for (int j = 0; j < UV_VI[i].rows(); j++)
		{
			if (UV_VI[i](j) == -1) continue;
			separated_V[i].row(UV_VI[i](j)) = V.row(id_uv_mesh.at(j));
		}

		VectorXd M, UV_M;
		doublearea(separated_V[i], separated_F_UV[i], M);
		doublearea(separated_V_UV[i], separated_F_UV[i], UV_M);

		total_area += M.sum();
		total_uv_area += UV_M.sum();
		chart_factors[i] = std::sqrt(UV_M.sum() / M.sum());
		//d_.add_new_chart(separated_V[i], separated_F_UV[i], separated_V_UV[i], separated_F_UV[i]);
	}

	double max_factor = chart_factors.maxCoeff();

	double avg_factor = std::sqrt(total_uv_area / total_area);
	d_.atlas_factor = max_factor / avg_factor;
	//std::cout << "Factor: Max " << chart_factors.maxCoeff() << ", Min " << chart_factors.minCoeff() << ", Avg " << atlas_factor << std::endl;
	for (int i = 0; i < component_number; i++)
	{
		separated_V[i] *= max_factor;
		d_.add_new_chart(separated_V[i], separated_F_UV[i], separated_V_UV[i], separated_F_UV[i]);
	}

	d_.bigger_factor_init = std::max(1.005, 0.9*d_.atlas_factor);
	//cout << "Atlas_factor: " << d_.atlas_factor << endl;
	return;

}


void StateManager::perform_one_iteration(const double &bigger_factor, double & last_mesh_energy, double & conv_rate)
{
	if (scaf_data.is_gap)
	{
		scaf_data.mesh_improve_inner(true, bigger_factor);
	}
	else
	{
		scaf_data.mesh_improve(true, bigger_factor);
	}

	parafun_solver->adjust_scaf_weight((last_mesh_energy)*scaf_data.mesh_measure / (scaf_data.sf_num) / 100.0);
	parafun_solver->after_mesh_improve();

	scaf_data.energy = parafun_solver->perform_iteration_cm(true);

	double current_mesh_energy = parafun_solver->compute_energy(scaf_data.w_uv, false) / scaf_data.mesh_measure - 4.0;
	double mesh_energy_decrease = last_mesh_energy - current_mesh_energy;

	conv_rate = abs(mesh_energy_decrease) / last_mesh_energy;
	last_mesh_energy = current_mesh_energy;
}
