#include <iostream>
#include <iomanip>
#include <fstream>
#include <queue>
#include "ChartDeformation.h"

#include "Common\CommonFunctions.h"
#include "Optimization\Numeric\FastMath.h"
#include "Mosek\ConvexQuadOptimization.h"

#include "ChartUntangle.h"

#include <QElapsedTimer>

ChartDeformation::ChartDeformation(PolySquareDeformation& _parent)
	: parent(_parent)
{
}

ChartDeformation::~ChartDeformation()
{
}

void ChartDeformation::build_chart()
{
	const auto& para = parent.para;
	n_boundary_edges = boundary_h_meshid.size();

	boundary_h_vert.resize(n_boundary_edges);
	boundary_h_tag.resize(n_boundary_edges);
	boundary_h_svec.resize(n_boundary_edges);

	double total_blen = 0.0;
	for (double len : boundary_h_len0)
	{
		total_blen += len;
	}
	total_area = 0.0;
	total_boundary_length = total_blen;
	n_vertices = 0;
	n_edges = 0;
	n_faces = 0;
}

void ChartDeformation::build_vertices()
{
	const auto& para = parent.para;
	const auto& v_chart = parent.v_chart;

	uv_x.resize(n_vertices * 2);
	for (int j = 0; j < n_boundary_edges; j++)
	{
		auto h_h = para.halfedge_handle(boundary_h_meshid[j]);

		boundary_h_vert[j].first = v_chart[para.from_vertex_handle(h_h).idx()].second;
		boundary_h_vert[j].second = v_chart[para.to_vertex_handle(h_h).idx()].second;
	}

	boundary_v_k.assign(n_boundary_edges, 0);
}

void ChartDeformation::build_faces()
{
	const auto& v_chart = parent.v_chart;

	double total_uv_area = 0.0;
	chart_face_info.reserve(mesh_faces.size());
	for (int fid : mesh_faces)
	{
		chart_face_info.push_back(parent.mesh_face_info[fid]);

		auto& finfo = chart_face_info.back();
		finfo.uv0 = v_chart[finfo.uv0].second;
		finfo.uv1 = v_chart[finfo.uv1].second;
		finfo.uv2 = v_chart[finfo.uv2].second;

		auto p1 = get_vec(finfo.uv0, finfo.uv1, uv_x);
		auto p2 = get_vec(finfo.uv0, finfo.uv2, uv_x);

		total_uv_area += 0.5 * (p1[0] * p2[1] - p1[1] * p2[0]);
	}

	z_pos = (chart_face_info[0].normal_towards == 1);

	scale_finfo(std::sqrt(total_uv_area / total_area));
}

double ChartDeformation::interior_angle(int next_boundary_h, const std::vector<double>& uv)
{
	const auto& para = parent.para;
	const auto& v_chart = parent.v_chart;

	auto h_h = para.halfedge_handle(boundary_h_meshid[next_boundary_h]);

	double angle = 0.0;
	int v0 = boundary_h_vert[next_boundary_h].first;
	auto h_iter = para.opposite_halfedge_handle(h_h);
	while (!para.is_boundary(h_iter))
	{
		int v1 = v_chart[para.from_vertex_handle(h_iter).idx()].second;
		h_iter = para.next_halfedge_handle(h_iter);
		int v2 = v_chart[para.to_vertex_handle(h_iter).idx()].second;

		OpenMesh::Vec2d p1 = get_vec(v0, v1, uv);
		OpenMesh::Vec2d p2 = get_vec(v0, v2, uv);
		angle += CommonFunctions::vec_angle_atan2(p1, p2);
		
		h_iter = para.opposite_halfedge_handle(h_iter);
	}

	return std::abs(angle);
}

ChartDeformation::corner ChartDeformation::next_corner(corner it)
{
	if (it->status & 4) return it;
	auto ith = boundary_corners.begin();
	auto itt = boundary_corners.end();

	do 
	{
		++it;
		if (it == itt) it = ith;
	} while (it->status & 4);

	return it;
}

ChartDeformation::corner ChartDeformation::prev_corner(corner it)
{
	if (it->status & 4) return it;
	auto ith = boundary_corners.begin();
	auto itt = boundary_corners.end();

	do
	{
		if (it == ith) it = itt;
		--it;
	} while (it->status & 4);

	return it;
}

void ChartDeformation::calc_seglength(corner it, const std::vector<double>& bh_len)
{
	it->seg_length = 0.0;
	auto next_it = next_corner(it);
	for (int bh = it->bvid; bh != next_it->bvid; bh = boundary(bh + 1))
	{
		it->seg_length += bh_len[bh];
	}
}

void ChartDeformation::update_segs_len(int uv_id)
{
	std::vector<double> bh_len;
	if (uv_id >= 0)
	{
		const auto& uv = (uv_id < uv_x_vec.size()) ? uv_x_vec[uv_id] : uv_x;

		bh_len.resize(n_boundary_edges);
		for (int i = 0; i < n_boundary_edges; i++)
		{
			int v0 = boundary_h_vert[i].first;
			int v1 = boundary_h_vert[i].second;

			bh_len[i] = get_vec(v0, v1, uv).norm();
		}
	}

	const std::vector<double>& bh_len_ref = (uv_id == -1) ? boundary_h_len0 : bh_len;
	for (auto it = boundary_corners.begin(); it != boundary_corners.end(); it++)
	{
		calc_seglength(it, bh_len_ref);
	}
}

void ChartDeformation::deformation()
{
	begin_deformation();
	rotation_step();
	for (int i = 0; i < 15; ++i)
	{
		align_step(i);
		if (inner_step()) break;
	}
	flattening_step();
	untangle_step();
	final_step();
}

void ChartDeformation::begin_deformation()
{
	kernel_width = parent.kernel_width * (double)n_boundary_edges / 1000.0;
	kernel_width = std::min(std::max(min_sigma, kernel_width), max_sigma);

	align_alpha = 0.3;
	amips_alpha = 0.5;

	std::cout << "--------------------------------------------------" << std::endl;
	std::cout << "Deformation Chart... Boundary " << n_boundary_edges << " Kernel " << kernel_width << std::endl;

	build_faces();
	calc_boundary_directions();
}

void ChartDeformation::rotation_step()
{
	calc_global_rotation();
	uv_x_vec.push_back(uv_x);

	find_corners();
//	tag is invalid from here
	modify_short_segments();
	conflicting_segments();
	move_corners();

	tag_from_segments();
	hessian_preparation();
	energy_lambda = 1.0;
	if (parent.amips_exp) calc_align_energy<true>();
	else calc_align_energy<false>();
	std::cout << std::setw(4) << "Iter" << std::setw(18) << "Lambda"
		<< std::setw(15) << "E_align" << std::setw(15) << "E_iso" 
		<< std::setw(15) << "Max Diff" << std::setw(10) << "Changed" << std::endl;
	energy_lambda = 1.0 / (energy_align + 1e-8);
}

void ChartDeformation::align_step(int iteration)
{
	std::cout << std::setw(4) << iteration + 1;
	if (parent.amips_exp) calc_align_deformation<true>(500);
	else calc_align_deformation<false>(500);
	std::cout << "-A";
	uv_x_vec.push_back(uv_x);
}

bool ChartDeformation::inner_step()
{
	CM_inner_deformation(5);
	std::cout << "-I";
	uv_x_vec.push_back(uv_x);
	if (parent.amips_exp) calc_align_energy<true>();
	else calc_align_energy<false>();

	const bool seg_changed = modify_short_segments();
	const bool converged = !seg_changed && max_angle_align < 0.3;
	if (!converged)
	{
		update_segs_len(uv_x_vec.size());
		tag_from_segments();
		energy_lambda *= seg_changed ? 3.0 : 6.0;
	}
	std::cout << std::endl;
	return converged;
}

void ChartDeformation::flattening_step()
{
	fixed_uv_x.clear();
	update_segs_len();
	built_segments();
	segment_flattening();
	uv_x_vec.push_back(uv_x);
}

void ChartDeformation::untangle_step()
{
	ChartUntangle(*this).calc();
	uv_x_vec.push_back(uv_x);
}

void ChartDeformation::final_step()
{
	scale_finfo(1.0 / goal_length);
	if (parent.amips_exp) calc_final_deformation<true>(1000);
	else calc_final_deformation<false>(1000);
	scale_finfo(goal_length);
	for (int i : fixed_uv_x) uv_x[i] = std::round(uv_x[i]);
	uv_x_vec.push_back(uv_x);
	check_vk();
}

bool ChartDeformation::modify_short_segments(double thres_factor /*= 1.0*/)
{
	double avg_len0 = total_boundary_length / n_boundary_edges;
	double length_thres = thres_factor * kernel_width * avg_len0;
	struct queue_entry
	{
		corner it;
		double len;
		double score;
	};
	struct queue_comparer
	{
		constexpr bool operator ()(const queue_entry& a, const queue_entry& b) const
		{
			return (a.score == b.score) ? (a.len > b.len) : (a.score < b.score);
		}
	};
	std::priority_queue<queue_entry, std::vector<queue_entry>, queue_comparer> short_segments;
	auto angle_score = [&](int bv, int vk)
	{
		double t = (double)(2 - vk) - boundary_v_sangle[bv];
		return std::abs(t);
	};
	auto add_segment = [&](const corner& it)
	{
		if (it->seg_length >= length_thres) return;
		int vp = it->bvid;
		int vn = next_corner(it)->bvid;
		int vk0_p = boundary_v_k[vp];
		int vk0_n = boundary_v_k[vn];
		if ((vk0_p > 0) == (vk0_n > 0)) return;

		bool new_at_p = (std::abs(vk0_p) > std::abs(vk0_n));
		int vk1_p = new_at_p ? (vk0_p + vk0_n) : 0;
		int vk1_n = new_at_p ? 0 : (vk0_p + vk0_n);

		double score0 = angle_score(vp, vk0_p) + angle_score(vn, vk0_n);
		double score1 = angle_score(vp, vk1_p) + angle_score(vn, vk1_n);

		short_segments.emplace(queue_entry{ it, it->seg_length, score0 - score1 - 2.0 * it->seg_length / avg_len0 / kernel_width });
	};

	for (auto it = boundary_corners.begin(); it != boundary_corners.end(); it++) add_segment(it);

	while (!short_segments.empty())
	{
		auto it = short_segments.top().it;
		double len0 = short_segments.top().len;
		short_segments.pop();

		if ((it->status & 4) || it->seg_length != len0) continue;

		auto cp = prev_corner(it);
		auto cn = next_corner(it);
		int vk_p = boundary_v_k[it->bvid];
		int vk_n = boundary_v_k[cn->bvid];

		if ((vk_p > 0) == (vk_n > 0)) continue;

		if (vk_p + vk_n == 0)
		{
			boundary_v_k[it->bvid] = 0;
			boundary_v_k[cn->bvid] = 0;
			cp->seg_length += it->seg_length + cn->seg_length;

			it->status |= 4;
			cn->status |= 4;

			add_segment(cp);
		}
		else if (std::abs(vk_p) > std::abs(vk_n))
		{
			boundary_v_k[it->bvid] = vk_p + vk_n;
			boundary_v_k[cn->bvid] = 0;

			it->seg_length += cn->seg_length;

			it->status = ((it->status & 4) | (cn->status & 3));
			cn->status |= 4;
			add_segment(it);
		}
		else if (std::abs(vk_p) < std::abs(vk_n))
		{
			boundary_v_k[it->bvid] = 0;
			boundary_v_k[cn->bvid] = vk_p + vk_n;

			cp->seg_length += it->seg_length;

			it->status |= 4;
			add_segment(cp);
		}
	}

	for (auto it = boundary_corners.begin(); it != boundary_corners.end(); it++)
	{
		if ((it->status & 4) || it->seg_length >= length_thres) continue;

		auto cp = prev_corner(it);
		auto cn = next_corner(it);
		int vk_p = boundary_v_k[it->bvid];
		int vk_n = boundary_v_k[cn->bvid];

		if ((vk_p >= 0) || (vk_n >= 0)) continue;

		int vk_new = vk_p + vk_n;
		double score_p = std::abs(boundary_v_angle0[it->bvid] - (double)(2 - vk_new) * M_PI_2);
		double score_n = std::abs(boundary_v_angle0[cn->bvid] - (double)(2 - vk_new) * M_PI_2);
		if (score_p < score_n)
		{
			boundary_v_k[it->bvid] = vk_new;
			boundary_v_k[cn->bvid] = 0;

			it->seg_length += cn->seg_length;

			it->status = ((it->status & 4) | (cn->status & 3));
			cn->status |= 4;
		}
		else
		{
			boundary_v_k[it->bvid] = 0;
			boundary_v_k[cn->bvid] = vk_new;

			cp->seg_length += it->seg_length;

			it->status |= 4;
		}
	}

	bool has_seg_deleted = false;
	for (auto it = boundary_corners.begin(); it != boundary_corners.end();)
	{
		if (it->status & 4)
		{
			it = boundary_corners.erase(it);
			has_seg_deleted = true;
		}
		else ++it;
	}

	return has_seg_deleted;
}

bool ChartDeformation::conflicting_segments()
{
	bool no_neg_quad = true;

	double length_thres = kernel_width * total_boundary_length / n_boundary_edges;

	for (auto it = boundary_corners.begin(); it != boundary_corners.end(); it++)
	{
		if ((it->status & 4) || it->seg_length >= length_thres) continue;

		auto cp = prev_corner(it);
		auto cn = next_corner(it);
		int vk_p = boundary_v_k[it->bvid];
		int vk_n = boundary_v_k[cn->bvid];

		if ((vk_p != 1) || (vk_n != 1)) continue;

		if (cp->seg_length > cn->seg_length)
		{
			double thres = std::min(length_thres, cp->seg_length / 2.0);
			double dist = it->seg_length;

			int cur_id = it->bvid;
			while (dist < thres)
			{
				cur_id = boundary(cur_id - 1);
				dist += boundary_h_len0[cur_id];
			}
			boundary_v_k[it->bvid] = 0;
			it->bvid = cur_id;
			boundary_v_k[it->bvid] = 1;

			calc_seglength(cp, boundary_h_len0);
			calc_seglength(it, boundary_h_len0);
		}
		else
		{
			double thres = std::min(length_thres, cn->seg_length / 2.0);
			double dist = it->seg_length;

			int cur_id = cn->bvid;
			while (dist < thres)
			{
				dist += boundary_h_len0[cur_id];
				cur_id = boundary(cur_id + 1);
			}
			boundary_v_k[cn->bvid] = 0;
			cn->bvid = cur_id;
			boundary_v_k[cn->bvid] = 1;

			calc_seglength(cn, boundary_h_len0);
			calc_seglength(it, boundary_h_len0);
		}
	}

	for (auto it = boundary_corners.begin(); it != boundary_corners.end(); it++)
	{
		int k0 = boundary_v_k[it->bvid];
		no_neg_quad = no_neg_quad && (k0 <= 2);
		if (k0 <= 1 || k0 > 2) continue;
		
		auto cp = prev_corner(it);
		auto cn = next_corner(it);
		if (cp->seg_length > it->seg_length)
		{
			double thres = std::min(length_thres, cp->seg_length / 2.0);
			double dist = 0.0;
			auto new_corner = boundary_corners.emplace(it);
			new_corner->status = (cp->status - 1) & 3;

			int cur_id = it->bvid;
			while (dist < thres)
			{
				cur_id = boundary(cur_id - 1);
				dist += boundary_h_len0[cur_id];
			}
			new_corner->bvid = cur_id;

			calc_seglength(cp, boundary_h_len0);
			calc_seglength(new_corner, boundary_h_len0);

			boundary_v_k[it->bvid] = 1;
			boundary_v_k[new_corner->bvid] = 1;
		}
		else
		{
			double thres = std::min(length_thres, it->seg_length / 2.0);
			double dist = 0.0;
			auto new_corner = boundary_corners.emplace(it);
			new_corner->bvid = it->bvid;
			new_corner->status = (cp->status - 1) & 3;

			int cur_id = it->bvid;
			while (dist < thres)
			{
				dist += boundary_h_len0[cur_id];
				cur_id = boundary(cur_id + 1);
			}
			it->bvid = cur_id;

			calc_seglength(it, boundary_h_len0);
			calc_seglength(new_corner, boundary_h_len0);

			boundary_v_k[it->bvid] = 1;
			boundary_v_k[new_corner->bvid] = 1;
		}
	}

	if (!no_neg_quad) std::cout << "Unhandled Conflicts" << std::endl;
	return no_neg_quad;
}

void ChartDeformation::tag_from_segments()
{
	for (auto it = boundary_corners.begin(); it != boundary_corners.end(); it++)
	{
		auto next_it = next_corner(it);

		for (int bh = it->bvid; bh != next_it->bvid; bh = boundary(bh + 1))
		{
			boundary_h_tag[bh] = (it->status & 3);
		}
	}
}

void ChartDeformation::built_segments()
{
	segments.reserve(boundary_corners.size());
	for (auto cur_it = boundary_corners.begin(); cur_it != boundary_corners.end(); cur_it++)
	{
		segments.emplace_back();

		segments.back().begin = cur_it->bvid;
		segments.back().end = next_corner(cur_it)->bvid;
		segments.back().tag = boundary_h_tag[cur_it->bvid];
		segments.back().length0 = cur_it->seg_length;
		segments.back().size = boundary(segments.back().end - segments.back().begin);
	}
}

int ChartDeformation::boundary(int bid)
{
	return CommonFunctions::period_id(bid, n_boundary_edges);
}

void ChartDeformation::calc_boundary_directions()
{
	fixed_uv_x.clear();

	boundary_h_svec.resize(n_boundary_edges);
	boundary_v_angle0.resize(n_boundary_edges);
	for (int j = 0; j < n_boundary_edges; j++)
	{
		int v0 = boundary_h_vert[j].first;
		int v1 = boundary_h_vert[j].second;

		boundary_h_svec[j] = get_vec(v0, v1, uv_x);
		boundary_v_angle0[j] = interior_angle(j, uv_x);

		fixed_uv_x.insert(2 * v0 + 0);
		fixed_uv_x.insert(2 * v0 + 1);
	}
	update_uv2kkt();

	boundary_v_sangle = boundary_v_angle0;
	double avg_len = total_boundary_length / n_boundary_edges;
	double gaussian_delta = kernel_width * avg_len;
	double gaussian_thres = 3.0 * gaussian_delta;
	double delta_sqr = 2.0 * gaussian_delta * gaussian_delta;

	for (int i = 0; i < 3; i++)
	{
		std::vector<OpenMesh::Vec2d> boundary_svec(n_boundary_edges);
		for (int j = 0; j < n_boundary_edges; j++)
		{
			OpenMesh::Vec2d smooth_vec = boundary_h_svec[j];
			for (int step : {1, -1})
			{
				int cur_id = j;
				double dist = boundary_h_len0[cur_id] / 2.0;
				while (dist < gaussian_thres)
				{
					cur_id = boundary(cur_id + step);
					int vert_h = (step == 1) ? boundary_h_vert[cur_id].first : boundary_h_vert[cur_id].second;

					double alpha = std::min((gaussian_thres - dist) / boundary_h_len0[cur_id], 1.0);
					double dist_b = dist + boundary_h_len0[cur_id] * alpha / 2.0;

					smooth_vec += ig::FastNegExp1(dist_b * dist_b / delta_sqr) * boundary_h_svec[cur_id] * alpha;

					dist += boundary_h_len0[cur_id];
				}
			}

			if (OpenMesh::dot(smooth_vec, boundary_h_svec[j]) < 0.0)
			{
				boundary_svec[j] = boundary_h_svec[j];
			}
			else
			{
				int k = boundary(j + 1);
				double d_angle = CommonFunctions::vec_angle_atan2(boundary_h_svec[j], smooth_vec);

				boundary_v_sangle[j] += d_angle;
				boundary_v_sangle[k] -= d_angle;
				boundary_svec[j] = smooth_vec.normalized() * boundary_h_svec[j].norm();
			}
		}
		boundary_h_svec = std::move(boundary_svec);
	}

	for (int j = 0; j < n_boundary_edges; j++) boundary_h_svec[j].normalize();
// 
// 	std::cout << "SUM ANGLE " << 2 * n_boundary_edges - std::accumulate(boundary_v_angle0.begin(), boundary_v_angle0.end(), 0.0) / M_PI_2 << std::endl;
// 	std::cout << "SUM SMOOTHED ANGLE " << 2 * n_boundary_edges - std::accumulate(boundary_v_sangle.begin(), boundary_v_sangle.end(), 0.0) / M_PI_2 << std::endl;
}

void ChartDeformation::find_corners()
{
	std::vector<double> v_angle2 = boundary_v_sangle;
	for (int j = 0; j < n_boundary_edges; j++)
	{
		auto angle = std::atan2(boundary_h_svec[j][1], boundary_h_svec[j][0]);
		int quad_angle = std::lround(angle / M_PI_2);
		angle -= (double)quad_angle * M_PI_2;

		int k = boundary(j + 1);

		v_angle2[j] -= angle;
		v_angle2[k] += angle;

		boundary_h_tag[j] = (quad_angle & 3);
	}

	for (int j = 0; j < n_boundary_edges; j++)
	{
		boundary_v_k[j] = 2 - std::lround(v_angle2[j] / M_PI_2);
	}

	boundary_corners.clear();
	for (int j = 0; j < n_boundary_edges; j++)
	{
		if (boundary_v_k[j] != 0)
		{
			boundary_corners.emplace_back();
			boundary_corners.back().bvid = j;
			boundary_corners.back().status = boundary_h_tag[j];

			//std::cout << boundary_h_vert[j].first << ", " << boundary_h_vert[j].second << ", " << boundary_v_k[j] << ", " << boundary_h_tag[j] << std::endl;
		}
	}

	update_segs_len();
}

void ChartDeformation::move_corners()
{
	double dist_thres = 2.0 * kernel_width * total_boundary_length / n_boundary_edges;

	auto k_diff = [&](int bvid, int k0) {return boundary_v_angle0[bvid] / M_PI_2 - (double)(2 - k0); };

	auto moving_condition_pos = [&](int bvid, int k0) { return boundary_v_angle0[bvid] / M_PI_2 > 1.5; };
	auto moving_condition_neg = [&](int bvid, int k0) { return std::abs(k_diff(bvid, k0)) > 0.5; };
	auto candidate_condition_pos = [&](int bvid, int k0) { return boundary_v_angle0[bvid] / M_PI_2 < 1.4; };
	auto candidate_condition_neg = [&](int bvid, int k0) { return std::abs(k_diff(bvid, k0)) < 0.4; };

	for (auto it = boundary_corners.begin(); it != boundary_corners.end(); it++)
	{
		int new_id = -1;
		auto cp = prev_corner(it);
		double min_dist = std::numeric_limits<double>::max();

		int k0 = boundary_v_k[it->bvid];
		if (boundary_v_angle0[it->bvid] > 1.75 * M_PI) continue;
		if ((k0 > 0 && !moving_condition_pos(it->bvid, k0)) || (k0 < 0 && !moving_condition_neg(it->bvid, k0))) continue;

		for (int step : {1, -1})
		{
			int cur_id = it->bvid;
			double dist = 0.0;
			double thres0 = (step == 1) ? (it->seg_length - dist_thres * 0.5) : (cp->seg_length - dist_thres * 0.5);
			thres0 = std::min(thres0, dist_thres);
			while (dist < thres0)
			{
				cur_id = boundary(cur_id + step);
				if (boundary_v_k[cur_id] != 0) break;
				if ((k0 > 0 && candidate_condition_pos(cur_id, k0)) || (k0 < 0 && candidate_condition_neg(cur_id, k0))) break;
				dist += boundary_h_len0[(step == 1) ? boundary(cur_id - 1) : cur_id];
			}

			if (boundary_v_k[cur_id] == 0 && dist < thres0 && dist < min_dist)
			{
				min_dist = dist;
				new_id = cur_id;
			}
		}

		if (new_id != -1)
		{
			boundary_v_k[new_id] = boundary_v_k[it->bvid];
			boundary_v_k[it->bvid] = 0;
			it->bvid = new_id;

			calc_seglength(cp, boundary_h_len0);
			calc_seglength(it, boundary_h_len0);
		}
	}
}

void ChartDeformation::segment_flattening()
{
	auto vf = [&](int bh) {return boundary_h_vert[bh].first; };
	auto vt = [&](int bh) {return boundary_h_vert[bh].second; };
	for (int i = 0; i < 2 * n_vertices; i++)
	{
		uv_x[i] /= goal_length;
	}

	std::vector<std::vector<double>> seg_pos(segments.size());
	for (int i = 0; i < segments.size(); i++)
	{
		auto& seg = segments[i];
		seg_pos[i].reserve(seg.size);
		double len_accu = 0.0;
		double w_coord = 0.0;
		int coord_tag = ((seg.tag & 1) ^ 1);
		for (int bh = seg.begin; bh != seg.end; bh = boundary(bh + 1))
		{
			int v0 = vf(bh);
			int v1 = vt(bh);

			w_coord += (uv_x[2 * v0 + coord_tag] + uv_x[2 * v1 + coord_tag]) / 2.0 * boundary_h_len0[bh];

			len_accu += get_vec(v0, v1, uv_x).norm();
			seg_pos[i].emplace_back(len_accu);
		}
		seg.coordinate = w_coord / seg.length0;
		for (double& pos : seg_pos[i]) pos /= len_accu;
	}

	std::vector<double> sol_x;
	mosek_flattening(sol_x);

	auto set_rounding = [&](int id, double val)
	{
		uv_x[id] = val;
		fixed_uv_x.insert(id);
	};

	for (double& val : sol_x) val = std::round(val);
	
	for (int i = 0; i < segments.size(); i++)
	{
		auto& seg = segments[i];

		if (boundary_v_k[seg.begin] != 0)
		{
			int v0 = vf(seg.begin);
			set_rounding(2 * v0 + 0, sol_x[2 * i + 0]);
			set_rounding(2 * v0 + 1, sol_x[2 * i + 1]);
		}

		int coord_tag = ((seg.tag & 1) ^ 1);
		seg.coordinate = sol_x[2 * i + coord_tag];

		for (int bh = seg.begin; bh != seg.end; bh = boundary(bh + 1))
		{
			set_rounding(2 * vf(bh) + coord_tag, seg.coordinate);
		}
	}

	for (int i = 0; i < segments.size(); i++)
	{
		auto& seg = segments[i];
		int tag = seg.tag & 1;
		double seg_length = uv_x[2 * vf(seg.end) + tag] - uv_x[2 * vf(seg.begin) + tag];
		double uv_begin = uv_x[2 * vf(seg.begin) + tag];

		int j = 0;
		int bh_end = boundary(seg.end - 1);
		for (int bh = seg.begin; bh != bh_end; bh = boundary(bh + 1))
		{
			uv_x[2 * vt(bh) + tag] = uv_begin + seg_pos[i][j++] * seg_length;
		}
	}

	update_uv2kkt();
}

bool ChartDeformation::mosek_flattening(std::vector<double>& sol_x)
{
	/*---------------------------------------------------------------

		min 1/2*x^T*Q*x + c^T*x
		s.t. blc <= A*x <= buc

		x: vector x, s.t. blx <= x <= bux
			bkx: bound type of each xi
				MSK_BK_LO: blxi <= xi <= +Inf (lower bound)
				MSK_BK_UP: -Inf <= xi <= buxi (upper bound)
				MSK_BK_FX: blxi == xi == buxi (fixed)
				MSK_BK_FR: -Inf <= xi <= +Inf (free)
				MSK_BK_RA: blxi <= xi <= buxi (ranged)
		c: vector c
		Q: sparse representation
			Q(qsubi, qsubj) = qval

		constraints:
		bkc, blc, buc: similar to bkx, blx, bux
		A: sparse representation, different from Q

	---------------------------------------------------------------*/

	std::map<int, int> bv2seg;
	for (int i = 0; i < segments.size(); i++) bv2seg[boundary_h_vert[segments[i].begin].first] = i;

	int n_segs = segments.size();

	std::vector<double> c(n_segs * 2, 0.0);
	
	std::vector<MSKidxt> qsubi(n_segs * 2);
	std::vector<MSKidxt> qsubj(n_segs * 2);
	std::vector<double> qval(n_segs * 2);
	
	std::vector<MSKboundkeye> bkx(n_segs * 2, MSK_BK_FR);
	std::vector<double> blx(n_segs * 2, -MSK_INFINITY);
	std::vector<double> bux(n_segs * 2, +MSK_INFINITY);
	
	std::vector<MSKboundkeye> bkc;
	std::vector<double> blc;
	std::vector<double> buc;
	
	std::vector<MSKlidxt> aptrb(n_segs * 2 + 1);
	std::vector<MSKidxt> asub;
	std::vector<double> aval;

	double lambda = 0.001 * total_boundary_length / n_boundary_edges;
	for (int i = 0; i < n_segs; i++)
	{
		qsubi[2 * i + 0] = 2 * i + 0;
		qsubj[2 * i + 0] = 2 * i + 0;

		qsubi[2 * i + 1] = 2 * i + 1;
		qsubj[2 * i + 1] = 2 * i + 1;

		int tag0 = segments[i].tag & 1;
		int tag1 = tag0 ^ 1;

		qval[2 * i + tag0] = lambda;
		qval[2 * i + tag1] = segments[i].length0;

		c[2 * i + tag0] = -lambda * uv_x[2 * boundary_h_vert[segments[i].begin].first + tag0];
		c[2 * i + tag1] = -segments[i].length0 * segments[i].coordinate;
	}

	int n_constraints = n_segs * 2;

	bkc.resize(n_constraints);
	blc.resize(n_constraints);
	buc.resize(n_constraints);

	std::vector<Eigen::Triplet<int>> constraints_sparse;
	constraints_sparse.reserve(n_constraints * 2);
	for (int i = 0; i < n_segs; i++)
	{
		int tag = segments[i].tag;
		int coord_tag = (tag & 1) ^ 1;
		int j = CommonFunctions::period_id(i + 1, segments.size());

		constraints_sparse.emplace_back(i, 2 * i + coord_tag, +1);
		constraints_sparse.emplace_back(i, 2 * j + coord_tag, -1);

		bkc[i] = MSK_BK_FX;
		blc[i] = 0.0;
		buc[i] = 0.0;

		int i_sign = (tag & 2) ? +1 : -1;
		constraints_sparse.emplace_back(i + n_segs, 2 * i + (tag & 1), +i_sign);
		constraints_sparse.emplace_back(i + n_segs, 2 * j + (tag & 1), -i_sign);

		double len_seg = get_vec(boundary_h_vert[segments[i].begin].first, boundary_h_vert[segments[j].begin].first, uv_x).norm();
		bkc[i + n_segs] = MSK_BK_LO;
		blc[i + n_segs] = std::max(2.0, 0.9 * len_seg);
		buc[i + n_segs] = +MSK_INFINITY;
	}

	Eigen::SparseMatrix<int> constraints_mat(n_constraints, n_segs * 2);
	constraints_mat.setFromTriplets(constraints_sparse.begin(), constraints_sparse.end());
	constraints_mat.makeCompressed();

	asub.reserve(n_constraints * 2);
	aval.reserve(n_constraints * 2);

	aptrb[0] = 0;
	for (int i = 0; i < n_segs * 2; i++)
	{
		aptrb[i + 1] = aptrb[i];
		for (Eigen::SparseMatrix<int>::InnerIterator it(constraints_mat, i); it; ++it)
		{
			asub.push_back(it.row());
			aval.push_back(it.value());

			aptrb[i + 1]++;
		}
	}

	sol_x.resize(n_segs * 2);
	return solveConvexQuadPorgramming_mosek(bkc, blc, buc, bkx, blx, bux, aptrb, asub, aval, qsubi, qsubj, qval, c, sol_x);
}

void ChartDeformation::polysquare_post_deformation()
{
	uv_x_vec.push_back(uv_x);
	uv_x_vec.push_back(uv_x);
	uv_x_vec.push_back(uv_x);

	auto get_tag = [](const OpenMesh::Vec2d& vec)
	{
		uint b = (abs(vec[0]) <= abs(vec[1]));
		uint a = (vec[b] <= 0.0);

		return (a << 1) + b;
	};

	fixed_uv_x.clear();
	for (int j = 0; j < n_boundary_edges; j++)
	{
		int v_p = boundary_h_vert[boundary(j - 1)].first;
		int v_c = boundary_h_vert[j].first;
		int v_n = boundary_h_vert[j].second;

		int tag_p = get_tag(get_vec(v_p, v_c, uv_x));
		int tag_n = get_tag(get_vec(v_c, v_n, uv_x));
		
		if (tag_p != tag_n)
		{
			fixed_uv_x.insert(2 * v_c + 0);
			fixed_uv_x.insert(2 * v_c + 1);
		}
		else
		{
			int tag_uv = ((tag_p & 1) ^ 1);
			fixed_uv_x.insert(2 * v_c + tag_uv);
		}
	}
	update_uv2kkt();

	ChartDeformation::calc_final_deformation<true>(1000);
	for (int i : fixed_uv_x) uv_x[i] = std::round(uv_x[i]);
	std::cout << std::endl;

	uv_x_vec.push_back(uv_x);
	uv_x_vec.push_back(uv_x);
	uv_x_vec.push_back(uv_x);
}

void ChartDeformation::hessian_preparation()
{
	H_det.setZero();
	H_det(0, 4) = 1.0;
	H_det(0, 5) = -1.0;
	H_det(1, 3) = -1.0;
	H_det(1, 5) = 1.0;
	H_det(2, 3) = 1.0;
	H_det(2, 4) = -1.0;

	H_det(3, 1) = -1.0;
	H_det(3, 2) = 1.0;
	H_det(4, 0) = 1.0;
	H_det(4, 2) = -1.0;
	H_det(5, 0) = -1.0;
	H_det(5, 1) = 1.0;
}

void ChartDeformation::update_uv2kkt()
{
	uv2kkt.assign(2 * n_vertices, -2);
	for (int i : fixed_uv_x) uv2kkt[i] = -1;
	
	int n_kkt = 0;
	for (int i = 0; i < 2 * n_vertices; i++)
	{
		if (uv2kkt[i] == -2) uv2kkt[i] = (n_kkt++);
	}
}

void ChartDeformation::update_face_info()
{
// 	for (auto& finfo : chart_face_info)
// 	{
// 		int v0 = finfo.uv0;
// 		int v1 = finfo.uv1;
// 		int v2 = finfo.uv2;
// 
// 		auto p1 = get_vec(v0, v1, uv_x);
// 		auto p2 = get_vec(v0, v2, uv_x);
// 
// 		finfo.l2_p1 = p1.sqrnorm();
// 		finfo.l2_p2 = p2.sqrnorm();
// 		finfo.dot_p = OpenMesh::dot(p1, p2);
// 		finfo.det_p = p1[0] * p2[1] - p1[1] * p2[0];
// 	}
}

void ChartDeformation::scale_finfo(double scale)
{
	double scale2 = scale * scale;
	for (auto& finfo : chart_face_info)
	{
		finfo.l2_p1 *= scale2;
		finfo.l2_p2 *= scale2;
		finfo.dot_p *= scale2;
		finfo.det_p *= scale2;
	}

// 	total_boundary_length *= scale;
 	total_area *= scale2;
}

void ChartDeformation::check_vk()
{
	for (int j = 0; j < n_boundary_edges; j++)
	{
		auto v_h = parent.para.from_vertex_handle(parent.para.halfedge_handle(boundary_h_meshid[j]));
		double angle = interior_angle(j, uv_x);
		int quad0 = 2 - boundary_v_k[j];
		int quad1 = std::lround(angle / M_PI_2);

		if (quad0 != quad1) std::cout << "K error at " << v_h.idx() << ", " << quad1 << "/" << quad0 << std::endl;
	}
}
