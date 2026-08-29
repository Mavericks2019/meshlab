
#ifndef SCAFFOLD_TEST_STATEMANAGER_H
#define SCAFFOLD_TEST_STATEMANAGER_H

#include "ScafCommon.h"
#include "ScafData.h"
#include <string>
#include <memory>
#include <iostream>
#include <functional>
#include "Parafun.h"

struct StateManager
{
  using IterationCallback = std::function<void(const Eigen::MatrixXd&, int)>;

  StateManager(){} //empty constructor

  void run_cmd(bool isfirst=true);
  //void run_cmd_second(Eigen::MatrixXd& uv_origin, double&src_distortion, ofstream &ofs);
  void perform_one_iteration(const double &bigger_factor, double &last_mesh_energy, double &conv_rate);

  void run_cmd_2bound(bool isfirst = true);
 
  void run(std::string filename, std::string filename_e, double gap,const string& type);

  void run_interface(const Eigen::MatrixXd& v_pos, const Eigen::MatrixXd& uv_v_pos, Eigen::MatrixXi& fv_id, Eigen::MatrixXi& uv_fv_id, double dis_bound, double gap, double peb = 0.8, const string& type = "LOWER", IterationCallback iteration_callback = {});

  void update_uv();
  void after_load();
  void atlas_init(std::string filename, ScafData& d_);

  void atlas_init_interface(const Eigen::MatrixXd& v_pos, const Eigen::MatrixXd& uv_v_pos, const Eigen::MatrixXi& fv_id,const Eigen::MatrixXi& uv_fv_id);
  const Eigen::MatrixXd& get_uv() { return scaf_data.UV_V; };

  //data
  ScafData scaf_data;
  int iter_count = 0;
  std::string model_file = std::string("NA");

  std::shared_ptr<Parafun> parafun_solver = nullptr;
  IterationCallback iteration_callback;

  //tweaking
  bool optimize_scaffold = true;
  bool predict_reference = false;
  bool fix_reference = false;

  enum class DemoType {
	  PACKING, FLOW, PARAM, BARS, ATLAS
  };

  DemoType demo_type= DemoType::ATLAS;

  bool demotype=true;

  double src_distortion;
  int iter_num_sum;
  struct OutRelated
  {
	  string gapstr;
	  string outtxtname;
	  string outobjstr;
	  string typestr;
	  double cur_dis;
	  double src_dis;
	  double time_sum;
	  //double cur_pe;
	  //double pe_bound;
  };

  OutRelated out_related;
  //display

  void notify_iteration();

};


#endif //SCAFFOLD_TEST_STATEMANAGER_H
