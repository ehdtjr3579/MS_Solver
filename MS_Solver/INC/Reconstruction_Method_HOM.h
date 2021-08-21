#pragma once
#include "Governing_Equation.h"
#include "Reconstruction_Method_FVM.h"
#include "Polynomial.h"


class HOM_Reconstruction : public RM {};

template <ushort space_dimension_, ushort solution_order_>
class Polynomial_Reconstruction : public HOM_Reconstruction
{
protected:
    using Space_Vector_ = Euclidean_Vector<space_dimension_>;

    static constexpr ushort num_basis_ = ms::combination_with_repetition(1 + space_dimension_, solution_order_);

protected:
    std::vector<Vector_Function<Polynomial<space_dimension_>, num_basis_>> basis_vector_functions_;

public:
    Polynomial_Reconstruction(const Grid<space_dimension_>& grid);

public:
    auto calculate_set_of_transposed_gradient_basis(void) const;
    auto calculate_basis_node(const uint cell_index, const Space_Vector_& node) const;
    Dynamic_Matrix calculate_basis_nodes(const uint cell_index, const std::vector<Space_Vector_>& nodes) const;
    double calculate_P0_basis_value(const uint cell_index, const Space_Vector_& node) const;

public:
    static constexpr ushort num_basis(void) { return num_basis_; };
    static constexpr ushort solution_order(void) { return solution_order_; };
    static constexpr ushort space_dimension(void) { return space_dimension_; };
    static std::string name(void) { return "Polynomial_Reconstruction_P" + std::to_string(solution_order_); };
};


class P1_Projected_MLP_Condition
{
public:
    static bool is_satisfy(const double P1_projected_value, const double allowable_min, const double allowable_max) {
        return allowable_min <= P1_projected_value && P1_projected_value <= allowable_max;
    }
};

class MLP_Smooth_Extrema_Detector
{
public:
    static bool is_smooth_extrema(const double solution, const double higher_mode_solution, const double P1_mode_solution, const double allowable_min, const double allowable_max) {
        if (P1_mode_solution > 0 && higher_mode_solution < 0 && solution > allowable_min)
            return true;
        else if (P1_mode_solution < 0 && higher_mode_solution > 0 && solution < allowable_max)
            return true;
        else
            return false;
    }
};


template <ushort space_dimension_, ushort solution_order_>
class hMLP_Base : public Polynomial_Reconstruction<space_dimension_, solution_order_>
{
private:
    static_require(1 <= solution_order_, "solution order should be greater than 1 for hMLP reconstruction");

    using This_ = hMLP_Base<space_dimension_, solution_order_>;

protected:
    static constexpr ushort num_basis_ = ms::combination_with_repetition(1 + space_dimension_, solution_order_);
    static constexpr ushort criterion_variable_index_ = 0;    

protected:
    std::unordered_map<uint, std::set<uint>> vnode_index_to_share_cell_indexes_;
    std::vector<std::vector<uint>> set_of_vnode_indexes_;
    std::array<ushort, solution_order_ + 1> num_Pn_projection_basis_;

protected:
    hMLP_Base(Grid<space_dimension_>&& grid);

protected:
    auto Pn_projection_matrix(const ushort Pn) const;
    auto Pn_mode_matrix(const ushort Pn) const;
    auto limiting_matrix(const double limiting_value) const;
};


template <ushort space_dimension_, ushort solution_order_>
class hMLP_Reconstruction : public hMLP_Base<space_dimension_, solution_order_>
{
private:
    using This_ = hMLP_Reconstruction<space_dimension_, solution_order_>;

public:
    static std::string name(void) { return "hMLP_Reconstruction_P" + std::to_string(solution_order_); };

protected:
    std::vector<Dynamic_Matrix> set_of_basis_vnodes_;
    std::vector<Dynamic_Matrix> set_of_P1_projected_basis_vnodes_;
    std::vector<double> P0_basis_values_;    
    
public:
    hMLP_Reconstruction(Grid<space_dimension_>&& grid);

public:
    template <ushort num_equation>
    void reconstruct(std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const;

private:
    template <ushort num_equation>
    auto calculate_P0_criterion_values(const std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const;
        
    auto calculate_vertex_node_index_to_allowable_min_max_criterion_value(const std::vector<double>& P0_criterion_values) const;

};


template <ushort space_dimension_, ushort solution_order_>
class hMLP_BD_Reconstruction : public hMLP_Base<space_dimension_, solution_order_>
{
private:
    static constexpr ushort num_P1_projected_basis_ = ms::combination_with_repetition(1 + space_dimension_, 1);
    
    using This_ = hMLP_BD_Reconstruction<space_dimension_, solution_order_>;

private:
    std::vector<Dynamic_Matrix> set_of_basis_vnodes_;
    std::vector<Dynamic_Matrix> set_of_simplex_P1_projected_basis_vnodes_;
    std::vector<Dynamic_Matrix> set_of_simplex_P0_projected_basis_vnodes_;

    std::vector<double> face_characteristic_lengths_;
    std::vector<std::pair<uint, uint>> face_oc_nc_index_pairs_;
    std::vector<std::pair<Dynamic_Matrix, Dynamic_Matrix>> face_oc_nc_side_basis_jump_qnodes_pairs_;
    std::vector<Dynamic_Euclidean_Vector> set_of_jump_qweights_;

public:
    hMLP_BD_Reconstruction(Grid<space_dimension_>&& grid);

public:
    template <ushort num_equation>
    void reconstruct(std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const;

private:
    template <ushort num_equation>
    auto calculate_set_of_vertex_node_index_to_simplex_P0_criterion_value(const std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const;

    auto calculate_vertex_node_index_to_allowable_min_max_criterion_value(const std::vector<std::map<uint, double>>& cell_index_to_vnode_index_to_simplex_P0_criterion_values) const;

    template <ushort num_equation>
    std::vector<ushort> calculate_set_of_num_trouble_boundary(const std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const;

    template <ushort projection_order>
    auto calculate_simplex_Pn_projection_basis_vector_function(const uint cell_index, const Geometry<space_dimension_>& sub_simplex_geometry) const;

    bool is_typeI_subcell_oscillation(const ushort num_trouble_boundaries) const;
    bool is_typeII_subcell_oscillation(const ushort num_trouble_boundaries) const;


public:
    static std::string name(void) { return "hMLP_BD_Reconstruction_P" + std::to_string(solution_order_); };
};


namespace ms {
    template <typename T>
    inline constexpr bool is_HOM_reconsturction_method = std::is_base_of_v<HOM_Reconstruction, T>;

    template <typename T>
    inline constexpr bool is_polynomial_reconustruction = std::is_same_v<Polynomial_Reconstruction<T::space_dimension(), T::solution_order()>, T>;

    template <typename Spatial_Discrete_Method, typename Reconstruction_Method>
    inline constexpr bool is_default_reconstruction<typename Spatial_Discrete_Method, typename Reconstruction_Method, std::enable_if_t<std::is_same_v<HOM, Spatial_Discrete_Method>>>
        = ms::is_polynomial_reconustruction<Reconstruction_Method>;
}


//template definition part
template <ushort space_dimension_, ushort solution_order_>
Polynomial_Reconstruction<space_dimension_, solution_order_>::Polynomial_Reconstruction(const Grid<space_dimension_>& grid) {
    SET_TIME_POINT;

    const auto& cell_elements = grid.elements.cell_elements;

    const auto num_cell = cell_elements.size();
    this->basis_vector_functions_.reserve(num_cell);

    for (uint i = 0; i < num_cell; ++i) {
        const auto& cell_element = cell_elements[i];
        const auto& cell_geometry = cell_element.geometry_;

        this->basis_vector_functions_.push_back(cell_geometry.orthonormal_basis_vector_function<solution_order_>());
    }

    Log::content_ << std::left << std::setw(50) << "@ Polynomial reconstruction precalculation" << " ----------- " << GET_TIME_DURATION << "s\n\n";
    Log::print();
}

template <ushort space_dimension_, ushort solution_order_>
auto Polynomial_Reconstruction<space_dimension_, solution_order_>::calculate_set_of_transposed_gradient_basis(void) const {
    const auto num_cell = this->basis_vector_functions_.size();
    std::vector<Matrix_Function<Polynomial<space_dimension_>, space_dimension_, this->num_basis_>> set_of_transposed_gradient_basis(num_cell);

    for (uint i = 0; i < num_cell; ++i) {
        auto& transposed_gradient_basis = set_of_transposed_gradient_basis[i];
        const auto& basis_function = this->basis_vector_functions_[i];

        for (ushort j = 0; j < this->num_basis_; ++j)
            transposed_gradient_basis.change_column(j, basis_function[j].gradient());
    }

    return set_of_transposed_gradient_basis;
}

template <ushort space_dimension_, ushort solution_order_>
auto Polynomial_Reconstruction<space_dimension_, solution_order_>::calculate_basis_node(const uint cell_index, const Space_Vector_& node) const {
    const auto& basis_functions = this->basis_vector_functions_[cell_index];
    return basis_functions(node);
}


template <ushort space_dimension_, ushort solution_order_>
Dynamic_Matrix Polynomial_Reconstruction<space_dimension_, solution_order_>::calculate_basis_nodes(const uint cell_index, const std::vector<Space_Vector_>& nodes) const {
    const auto& basis_functions = this->basis_vector_functions_[cell_index];

    const auto num_node = nodes.size();
    Dynamic_Matrix basis_nodes(this->num_basis_, num_node);

    for (ushort j = 0; j < num_node; ++j)
        basis_nodes.change_column(j, basis_functions(nodes[j]));

    return basis_nodes;
}

template <ushort space_dimension_, ushort solution_order_>
double Polynomial_Reconstruction<space_dimension_, solution_order_>::calculate_P0_basis_value(const uint cell_index, const Space_Vector_& node) const {
    const auto& basis_function = this->basis_vector_functions_[cell_index];
    const auto& P0_basis_function = basis_function[0];

    return P0_basis_function(node);
}


template <ushort space_dimension_, ushort solution_order_>
hMLP_Base<space_dimension_, solution_order_>::hMLP_Base(Grid<space_dimension_>&& grid)
    : Polynomial_Reconstruction<space_dimension_, solution_order_>(grid) {
    SET_TIME_POINT;

    for (ushort i = 0; i <= solution_order_; ++i)
        this->num_Pn_projection_basis_[i] = ms::combination_with_repetition(1 + space_dimension_, i);

    this->vnode_index_to_share_cell_indexes_ = std::move(grid.connectivity.vnode_index_to_share_cell_indexes);

    const auto num_cell = grid.elements.cell_elements.size();
    this->set_of_vnode_indexes_.reserve(num_cell);

    for (const auto& cell_element : grid.elements.cell_elements)
        this->set_of_vnode_indexes_.push_back(cell_element.vertex_node_indexes());


    Log::content_ << std::left << std::setw(50) << "@ hMLP Base precalculation" << " ----------- " << GET_TIME_DURATION << "s\n\n";
    Log::print();
}

template <ushort space_dimension_, ushort solution_order_>
auto hMLP_Base<space_dimension_, solution_order_>::Pn_projection_matrix(const ushort Pn) const {
    dynamic_require(Pn <= solution_order_, "Projection order should be less then solution order");

    const auto num_Pn_projection_basis = this->num_Pn_projection_basis_[Pn];

    std::array<double, This_::num_basis_> projection_value = { 0 };
    for (ushort j = 0; j < num_Pn_projection_basis; ++j)
        projection_value[j] = 1.0; //preserve Pn_projection_basis

    Matrix Pn_projection_matrix = projection_value;
    return Pn_projection_matrix;
}

template <ushort space_dimension_, ushort solution_order_>
auto hMLP_Base<space_dimension_, solution_order_>::Pn_mode_matrix(const ushort Pn) const {
    dynamic_require(Pn <= solution_order_, "mode should be less then solution order");

    const auto num_Pnm1_projection_basis = this->num_Pn_projection_basis_[Pn - 1];
    const auto num_Pn_projection_basis = this->num_Pn_projection_basis_[Pn];

    std::array<double, This_::num_basis_> projection_value = { 0 };
    for (ushort j = num_Pnm1_projection_basis; j < num_Pn_projection_basis; ++j)
        projection_value[j] = 1.0; //preserve Pn_mode_basis

    Matrix Pn_mode_matrix = projection_value;
    return Pn_mode_matrix;
}

template <ushort space_dimension_, ushort solution_order_>
auto hMLP_Base<space_dimension_, solution_order_>::limiting_matrix(const double limiting_value) const {
    std::array<double, This_::num_basis_> limiting_values;
    limiting_values.fill(limiting_value);
    limiting_values[0] = 1.0; // keep P0 coefficient
        
    return Matrix(limiting_values);
}

template <ushort space_dimension_, ushort solution_order_>
hMLP_Reconstruction<space_dimension_, solution_order_>::hMLP_Reconstruction(Grid<space_dimension_>&& grid) 
    : hMLP_Base<space_dimension_, solution_order_>(std::move(grid)){
    SET_TIME_POINT;

    const auto& cell_elements = grid.elements.cell_elements;

    const auto num_cell = cell_elements.size();
    this->set_of_basis_vnodes_.reserve(num_cell);
    this->set_of_P1_projected_basis_vnodes_.reserve(num_cell);
    this->P0_basis_values_.reserve(num_cell);

    constexpr ushort P0_basis_row_index = 0;
    constexpr ushort P1 = 1;

    for (uint i = 0; i < num_cell; ++i) {
        const auto& cell_geometry = cell_elements[i].geometry_;
        const auto vnodes = cell_geometry.vertex_nodes();

        auto basis_vnodes = this->calculate_basis_nodes(i, vnodes);
        auto P1_projected_basis_vnodes = this->Pn_projection_matrix(P1) * basis_vnodes;
        const auto P0_basis_value = P1_projected_basis_vnodes.at(P0_basis_row_index, 0);

        this->set_of_basis_vnodes_.push_back(std::move(basis_vnodes));
        this->set_of_P1_projected_basis_vnodes_.push_back(std::move(P1_projected_basis_vnodes));
        this->P0_basis_values_.push_back(P0_basis_value);
    }

    Log::content_ << std::left << std::setw(50) << "@ hMLP precalculation" << " ----------- " << GET_TIME_DURATION << "s\n\n";
    Log::print();
}


template <ushort space_dimension_, ushort solution_order_>
template <ushort num_equation>
void hMLP_Reconstruction<space_dimension_, solution_order_>::reconstruct(std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const {
    const auto P0_criterion_values = this->calculate_P0_criterion_values(solution_coefficients);
    const auto vnode_index_to_allowable_min_max_criterion_value = this->calculate_vertex_node_index_to_allowable_min_max_criterion_value(P0_criterion_values);

    const auto num_cell = solution_coefficients.size();

    for (uint i = 0; i < num_cell; ++i) {
        auto temporal_solution_order = solution_order_;

        auto& solution_coefficient = solution_coefficients[i];
        const auto& basis_vnodes = this->set_of_basis_vnodes_[i];
        const auto& P1_projected_basis_vnodes = this->set_of_P1_projected_basis_vnodes_[i];

        auto solution_vnodes = solution_coefficient * basis_vnodes;
        auto P1_projected_solution_vnodes = solution_coefficient * P1_projected_basis_vnodes;

        const auto& vnode_indexes = this->set_of_vnode_indexes_[i];
        const auto num_vnode = vnode_indexes.size();

        const auto P0_criterion_value = P0_criterion_values[i];

        for (ushort j = 0; j < num_vnode; ++j) {
            const auto vnode_index = vnode_indexes[j];
            const auto [allowable_min, allowable_max] = vnode_index_to_allowable_min_max_criterion_value.at(vnode_index);

            const auto criterion_value = solution_vnodes.at(This_::criterion_variable_index_, j);
            const auto P1_projected_criterion_value = P1_projected_solution_vnodes.at(This_::criterion_variable_index_, j);

            const auto higher_mode_criterion_value = criterion_value - P1_projected_criterion_value;
            const auto P1_mode_criterion_value = P1_projected_criterion_value - P0_criterion_value;

            while (true) {
                if (P1_Projected_MLP_Condition::is_satisfy(P1_projected_criterion_value, allowable_min, allowable_max))
                    break;
                else {
                    if (MLP_Smooth_Extrema_Detector::is_smooth_extrema(criterion_value, higher_mode_criterion_value, P1_mode_criterion_value, allowable_min, allowable_max))
                        break;
                    else {
                        if (temporal_solution_order == 1) {
                            const auto limiting_value = MLP_u1_Limiting_Strategy::calculate_limiting_value(P1_mode_criterion_value, P0_criterion_value, allowable_min, allowable_max);
                            solution_coefficient *= this->limiting_matrix(limiting_value);

                            break;
                        }
                        else {
                            //limiting highest mode
                            solution_coefficient *= this->Pn_projection_matrix(--temporal_solution_order);

                            //re-calculate limited vnode_solution                            
                            solution_vnodes = solution_coefficient * basis_vnodes;
                            P1_projected_solution_vnodes = solution_coefficient * P1_projected_basis_vnodes;
                        }
                    }
                }    
            }

        }
    }
}

template <ushort space_dimension_, ushort solution_order_>
template <ushort num_equation>
auto hMLP_Reconstruction<space_dimension_, solution_order_>::calculate_P0_criterion_values(const std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const {
    const auto num_cell = solution_coefficients.size();
    std::vector<double> P0_criterion_values(num_cell);

    for (uint i = 0; i < num_cell; ++i) {
        const auto P0_coefficient = solution_coefficients[i].column(0);
        const auto P0_solution = P0_coefficient * this->P0_basis_values_[i];
        P0_criterion_values[i] = P0_solution[This_::criterion_variable_index_];
    }        

    return P0_criterion_values;
}

template <ushort space_dimension_, ushort solution_order_>
auto hMLP_Reconstruction<space_dimension_, solution_order_>::calculate_vertex_node_index_to_allowable_min_max_criterion_value(const std::vector<double>& P0_criterion_values) const {
    const auto num_vnode = this->vnode_index_to_share_cell_indexes_.size();

    std::unordered_map<uint, std::pair<double, double>> vnode_index_to_allowable_min_max_criterion_value;
    vnode_index_to_allowable_min_max_criterion_value.reserve(num_vnode);

    for (const auto& [vnode_index, share_cell_indexes] : this->vnode_index_to_share_cell_indexes_) {
        const auto num_share_cell = share_cell_indexes.size();
        std::vector<double> criterion_variables;
        criterion_variables.reserve(num_share_cell);

        for (const auto cell_index : share_cell_indexes)
            criterion_variables.push_back(P0_criterion_values[cell_index]);

        const auto min_criterion_value = *std::min_element(criterion_variables.begin(), criterion_variables.end());
        const auto max_criterion_value = *std::max_element(criterion_variables.begin(), criterion_variables.end());

        vnode_index_to_allowable_min_max_criterion_value.emplace(vnode_index, std::make_pair(min_criterion_value, max_criterion_value));
    }

    return vnode_index_to_allowable_min_max_criterion_value;
}


template <ushort space_dimension_, ushort solution_order_>
hMLP_BD_Reconstruction<space_dimension_, solution_order_>::hMLP_BD_Reconstruction(Grid<space_dimension_>&& grid) 
    : hMLP_Base<space_dimension_, solution_order_>(std::move(grid)) {
    SET_TIME_POINT;

    const auto& cell_elements = grid.elements.cell_elements;

    const auto num_cell = cell_elements.size();
    this->set_of_basis_vnodes_.reserve(num_cell);
    this->set_of_simplex_P1_projected_basis_vnodes_.reserve(num_cell);
    this->set_of_simplex_P0_projected_basis_vnodes_.reserve(num_cell);


    constexpr ushort P0 = 0;
    constexpr ushort P1 = 1;
    for (uint i = 0; i < num_cell; ++i) {
        const auto& cell_geometry = cell_elements[i].geometry_;

        const auto vnodes = cell_geometry.vertex_nodes();
        auto basis_vnodes = this->calculate_basis_nodes(i, vnodes);
                
        const auto& reference_geometry = cell_geometry.reference_geometry_;

        
        if (reference_geometry.is_simplex()) {
            auto P0_projected_basis_vnodes = this->Pn_projection_matrix(P0) * basis_vnodes;
            auto P1_projected_basis_vnodes = this->Pn_projection_matrix(P1) * basis_vnodes;

            this->set_of_simplex_P0_projected_basis_vnodes_.push_back(std::move(P0_projected_basis_vnodes));
            this->set_of_simplex_P1_projected_basis_vnodes_.push_back(std::move(P1_projected_basis_vnodes));            
        }
        else {
            const auto sub_simplex_geometries = cell_geometry.sub_simplex_geometries();

            const auto num_vnode = vnodes.size();
            Dynamic_Matrix simplex_P0_projected_basis_vnodes(This_::num_basis_, num_vnode);
            Dynamic_Matrix simplex_P1_projected_basis_vnodes(This_::num_basis_, num_vnode);

            for (ushort j = 0; j < num_vnode; ++j) {
                const auto simplex_P0_projection_basis_vector_function = this->calculate_simplex_Pn_projection_basis_vector_function<P0>(i, sub_simplex_geometries[j]);
                const auto simplex_P1_projection_basis_vector_function = this->calculate_simplex_Pn_projection_basis_vector_function<P1>(i, sub_simplex_geometries[j]);

                const auto& vnode = vnodes[j];

                simplex_P0_projected_basis_vnodes.change_column(j, simplex_P0_projection_basis_vector_function(vnode));
                simplex_P1_projected_basis_vnodes.change_column(j, simplex_P1_projection_basis_vector_function(vnode));
            }

            this->set_of_simplex_P0_projected_basis_vnodes_.push_back(std::move(simplex_P0_projected_basis_vnodes));
            this->set_of_simplex_P1_projected_basis_vnodes_.push_back(std::move(simplex_P1_projected_basis_vnodes));
        }

        this->set_of_basis_vnodes_.push_back(std::move(basis_vnodes));
    }


    const auto& inner_face_elements = grid.elements.inner_face_elements;
    const auto& inner_face_oc_nc_index_pairs = grid.connectivity.inner_face_oc_nc_index_pairs;
    const auto& pbdry_element_pairs = grid.elements.periodic_boundary_element_pairs;
    const auto& pbdry_oc_nc_index_pairs = grid.connectivity.periodic_boundary_oc_nc_index_pairs;
    
    const auto num_inner_face = inner_face_elements.size();
    const auto num_pbdry_pair = pbdry_element_pairs.size();

    const auto num_face_without_bdry = num_inner_face + num_pbdry_pair;
    this->face_characteristic_lengths_.reserve(num_face_without_bdry);
    this->face_oc_nc_index_pairs_.reserve(num_face_without_bdry);
    this->face_oc_nc_side_basis_jump_qnodes_pairs_.reserve(num_face_without_bdry);
    this->set_of_jump_qweights_.reserve(num_face_without_bdry);

    for (uint i = 0; i < num_inner_face; ++i) {
        const auto& geometry = inner_face_elements[i].geometry_;
        
        const auto charactersitic_length = std::pow(geometry.volume(), 1.0 / (space_dimension_ - 1));
        this->face_characteristic_lengths_.push_back(charactersitic_length);
        
        const auto [oc_index, nc_index] = inner_face_oc_nc_index_pairs[i];
        this->face_oc_nc_index_pairs_.push_back({ oc_index,nc_index });
        
        const auto& quadrature_rule = geometry.get_quadrature_rule(solution_order_);
        auto oc_side_basis_jump_qnodes = this->calculate_basis_nodes(oc_index, quadrature_rule.points);
        auto nc_side_basis_jump_qnodes = this->calculate_basis_nodes(nc_index, quadrature_rule.points);
        this->face_oc_nc_side_basis_jump_qnodes_pairs_.push_back({ std::move(oc_side_basis_jump_qnodes),std::move(nc_side_basis_jump_qnodes) });
        this->set_of_jump_qweights_.push_back(quadrature_rule.weights);        
    }

    for (uint i = 0; i < num_pbdry_pair; ++i) {
        const auto& [oc_side_element, nc_side_element] = pbdry_element_pairs[i];
        const auto& oc_side_geometry = oc_side_element.geometry_;
        const auto& nc_side_geometry = nc_side_element.geometry_;

        const auto characteristic_length = std::pow(oc_side_geometry.volume(), 1.0 / (space_dimension_ - 1));
        this->face_characteristic_lengths_.push_back(characteristic_length);

        const auto [oc_index, nc_index] = pbdry_oc_nc_index_pairs[i];
        this->face_oc_nc_index_pairs_.push_back({ oc_index,nc_index });


        const auto& oc_side_quadrature_rule = oc_side_geometry.get_quadrature_rule(solution_order_);
        const auto& oc_side_qnodes = oc_side_quadrature_rule.points;
        const auto& oc_side_qweights = oc_side_quadrature_rule.weights;

        const auto& nc_side_quadrature_rule = nc_side_geometry.get_quadrature_rule(solution_order_);
        const auto& nc_side_qnodes = nc_side_quadrature_rule.points;

        auto oc_side_basis_jump_qnodes = this->calculate_basis_nodes(oc_index, oc_side_qnodes);
        auto nc_side_basis_jump_qnodes = this->calculate_basis_nodes(nc_index, nc_side_qnodes);
        this->face_oc_nc_side_basis_jump_qnodes_pairs_.push_back({ std::move(oc_side_basis_jump_qnodes),std::move(nc_side_basis_jump_qnodes) });
        this->set_of_jump_qweights_.push_back(oc_side_qweights); // oc side qweights == nc side qweights
    }

    Log::content_ << std::left << std::setw(50) << "@ hMLP_BD precalculation" << " ----------- " << GET_TIME_DURATION << "s\n\n";
    Log::print();
}



template <ushort space_dimension_, ushort solution_order_>
template <ushort num_equation>
void hMLP_BD_Reconstruction<space_dimension_, solution_order_>::reconstruct(std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const {    
    const auto set_of_vnode_index_to_simplex_P0_criterion_value = this->calculate_set_of_vertex_node_index_to_simplex_P0_criterion_value(solution_coefficients);
    const auto vnode_index_to_allowable_min_max_criterion_value = this->calculate_vertex_node_index_to_allowable_min_max_criterion_value(set_of_vnode_index_to_simplex_P0_criterion_value);
    const auto set_of_num_trobule_boundary = this->calculate_set_of_num_trouble_boundary(solution_coefficients);

    const auto num_cell = solution_coefficients.size();

    for (uint i = 0; i < num_cell; ++i) {
        auto temporal_solution_order = solution_order_;

        auto& solution_coefficient = solution_coefficients[i];
        const auto& basis_vnodes = this->set_of_basis_vnodes_[i];
        const auto& simplex_P1_projected_basis_vnodes = this->set_of_simplex_P1_projected_basis_vnodes_[i];

        auto solution_vnodes = solution_coefficient * basis_vnodes;
        auto simplex_P1_projected_solution_vnodes = solution_coefficient * simplex_P1_projected_basis_vnodes;

        const auto& vnode_indexes = this->set_of_vnode_indexes_[i];
        const auto num_vnode = vnode_indexes.size();

        bool end_limiting = false;


        const auto num_trouble_boundary = set_of_num_trobule_boundary[i];

        const auto& vnode_index_to_simplex_P0_criterion_value = set_of_vnode_index_to_simplex_P0_criterion_value[i];

        for (ushort j = 0; j < num_vnode; ++j) {
            const auto vnode_index = vnode_indexes[j];
            const auto [allowable_min, allowable_max] = vnode_index_to_allowable_min_max_criterion_value.at(vnode_index);

            while (true) {
                const auto simplex_P1_projected_solution = simplex_P1_projected_solution_vnodes.column<num_equation>(j);
                const auto simplex_P1_projected_criterion_value = simplex_P1_projected_solution[This_::criterion_variable_index_];

                const auto simplex_P0_criterion_value = vnode_index_to_simplex_P0_criterion_value.at(vnode_index);
                const auto simplex_P1_mode_criterion_value = simplex_P1_projected_criterion_value - simplex_P0_criterion_value;

                if (P1_Projected_MLP_Condition::is_satisfy(simplex_P1_projected_criterion_value, allowable_min, allowable_max)) {
                    if (this->is_typeI_subcell_oscillation(num_trouble_boundary)) {
                        const auto limiting_value = MLP_u1_Limiting_Strategy::calculate_limiting_value(simplex_P1_mode_criterion_value, simplex_P0_criterion_value, allowable_min, allowable_max);
                        solution_coefficient *= this->limiting_matrix(limiting_value);

                        end_limiting = true;
                    }
                    break;
                }
                else {
                    const auto solution = solution_vnodes.column<num_equation>(j);

                    const auto criterion_value = solution[This_::criterion_variable_index_];
                    const auto higher_mode_criterion_value = criterion_value - simplex_P1_projected_criterion_value;

                    if (MLP_Smooth_Extrema_Detector::is_smooth_extrema(criterion_value, higher_mode_criterion_value, simplex_P1_mode_criterion_value, allowable_min, allowable_max) 
                        && !this->is_typeII_subcell_oscillation(num_trouble_boundary))
                            break;
                    else {
                        if (temporal_solution_order == 1) {
                            const auto limiting_value = MLP_u1_Limiting_Strategy::calculate_limiting_value(simplex_P1_mode_criterion_value, simplex_P0_criterion_value, allowable_min, allowable_max);
                            solution_coefficient *= this->limiting_matrix(limiting_value);

                            end_limiting = true;
                            break;
                        }
                        else {
                            //limiting highest mode
                            const auto Pnm1_projection_matrix = this->Pn_projection_matrix(--temporal_solution_order);
                            solution_coefficient *= Pnm1_projection_matrix;

                            //re-calculate limited vnode_solution                            
                            solution_vnodes = solution_coefficient * basis_vnodes;
                            simplex_P1_projected_solution_vnodes = solution_coefficient * simplex_P1_projected_basis_vnodes;
                        }
                    }
                }
            }

            if (end_limiting == true)
                break;
        }
    }
}


template <ushort space_dimension_, ushort solution_order_>
template <ushort num_equation>
auto hMLP_BD_Reconstruction<space_dimension_, solution_order_>::calculate_set_of_vertex_node_index_to_simplex_P0_criterion_value(const std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const {
    const auto num_cell = solution_coefficients.size();
    const auto num_vnode = this->vnode_index_to_share_cell_indexes_.size();

    std::vector<std::map<uint, double>> cell_index_to_vnode_index_to_simplex_P0_criterion_value;
    cell_index_to_vnode_index_to_simplex_P0_criterion_value.reserve(num_cell);

    for (uint i = 0; i < num_cell; ++i) {   
        std::map<uint, double> vnode_index_to_simplex_P0_criterion_value;

        const auto simplex_P0_projected_solution_vnodes = solution_coefficients[i] * this->set_of_simplex_P0_projected_basis_vnodes_[i];
        const auto simplex_P0_projected_criterion_variable_vnodes = simplex_P0_projected_solution_vnodes.row(This_::criterion_variable_index_);

        const auto& vnode_indexes = this->set_of_vnode_indexes_[i];
        const auto num_vnode = vnode_indexes.size();

        for (ushort j = 0; j < num_vnode; ++j)
            vnode_index_to_simplex_P0_criterion_value.emplace(vnode_indexes[j], simplex_P0_projected_criterion_variable_vnodes[j]);

        cell_index_to_vnode_index_to_simplex_P0_criterion_value.push_back(std::move(vnode_index_to_simplex_P0_criterion_value));
    }

    return cell_index_to_vnode_index_to_simplex_P0_criterion_value;
}

template <ushort space_dimension_, ushort solution_order_>
auto hMLP_BD_Reconstruction<space_dimension_, solution_order_>::calculate_vertex_node_index_to_allowable_min_max_criterion_value(const std::vector<std::map<uint, double>>& set_of_vnode_index_to_simplex_P0_criterion_value) const {
    const auto num_vnode = this->vnode_index_to_share_cell_indexes_.size();

    std::unordered_map<uint, std::vector<double>> vnode_index_to_simplex_P0_criterion_values;
    vnode_index_to_simplex_P0_criterion_values.reserve(num_vnode);

    for (const auto& [vnode_index, share_cell_indexes] : this->vnode_index_to_share_cell_indexes_)
        vnode_index_to_simplex_P0_criterion_values.emplace(vnode_index, std::vector<double>());

    for (const auto& vnode_index_to_simplex_P0_criterion_value : set_of_vnode_index_to_simplex_P0_criterion_value) {
        for (const auto& [vnode_index, simplex_P0_criterion_value] : vnode_index_to_simplex_P0_criterion_value)
            vnode_index_to_simplex_P0_criterion_values.at(vnode_index).push_back(simplex_P0_criterion_value);
    }

    std::unordered_map<uint, std::pair<double, double>> vnode_index_to_allowable_min_max_criterion_value;
    vnode_index_to_allowable_min_max_criterion_value.reserve(num_vnode);

    for (const auto& [vnode_index, simplex_P0_criterion_values] : vnode_index_to_simplex_P0_criterion_values) {
        const auto min_criterion_value = *std::min_element(simplex_P0_criterion_values.begin(), simplex_P0_criterion_values.end());
        const auto max_criterion_value = *std::max_element(simplex_P0_criterion_values.begin(), simplex_P0_criterion_values.end());

        vnode_index_to_allowable_min_max_criterion_value.emplace(vnode_index, std::make_pair(min_criterion_value, max_criterion_value));
    }

    return vnode_index_to_allowable_min_max_criterion_value;
}

template <ushort space_dimension_, ushort solution_order_>
template <ushort num_equation>
std::vector<ushort> hMLP_BD_Reconstruction<space_dimension_, solution_order_>::calculate_set_of_num_trouble_boundary(const std::vector<Matrix<num_equation, This_::num_basis_>>& solution_coefficients) const {
    const auto num_cell = solution_coefficients.size();

    std::vector<ushort> set_of_num_trouble_boundary(num_cell);

    const auto num_face = this->face_characteristic_lengths_.size();
    for (uint i = 0; i < num_face; ++i) {
        const auto characteristic_length = this->face_characteristic_lengths_[i];
        const auto jump_criterion = std::pow(characteristic_length, 0.5 * (solution_order_ + 1));

        const auto [oc_index, nc_index] = this->face_oc_nc_index_pairs_[i];
        const auto& [oc_side_basis_jump_qnodes, nc_side_basis_jump_qnodes] = this->face_oc_nc_side_basis_jump_qnodes_pairs_[i];

        const auto oc_side_solution_jump_qnodes = solution_coefficients[oc_index] * oc_side_basis_jump_qnodes;
        const auto nc_side_solution_jump_qnodes = solution_coefficients[nc_index] * nc_side_basis_jump_qnodes;

        const auto oc_side_cirterion_solution_jump_qnodes = oc_side_solution_jump_qnodes.row(This_::criterion_variable_index_);
        const auto nc_side_cirterion_solution_jump_qnodes = nc_side_solution_jump_qnodes.row(This_::criterion_variable_index_);
        auto criterion_solution_diff_jump_qnodes = oc_side_cirterion_solution_jump_qnodes - nc_side_cirterion_solution_jump_qnodes;
        criterion_solution_diff_jump_qnodes.be_absolute();

        const auto jump = criterion_solution_diff_jump_qnodes.inner_product(this->set_of_jump_qweights_[i]);

        if (jump > jump_criterion) {
            set_of_num_trouble_boundary[oc_index]++;
            set_of_num_trouble_boundary[nc_index]++;
        }
    }

    return set_of_num_trouble_boundary;
}

template <ushort space_dimension_, ushort solution_order_>
template <ushort projection_order>
auto hMLP_BD_Reconstruction<space_dimension_, solution_order_>::calculate_simplex_Pn_projection_basis_vector_function(const uint cell_index, const Geometry<space_dimension_>& sub_simplex_geometry) const {        
    constexpr auto Pk = projection_order;
    const auto& basis_vector_function = this->basis_vector_functions_[cell_index];
    const auto Pk_simplex_basis_vector_function = sub_simplex_geometry.orthonormal_basis_vector_function<Pk>(); // Pk := projection order
        
    std::array<Polynomial<space_dimension_>, This_::num_basis_> simplex_Pn_projection_basis_functions = { 0 };

    for (ushort i = 0; i < This_::num_basis_; ++i) 
        for (ushort j = 0; j < this->num_Pn_projection_basis_[Pk]; ++j) 
            simplex_Pn_projection_basis_functions[i] = ms::inner_product(basis_vector_function[i], Pk_simplex_basis_vector_function[j], sub_simplex_geometry) * Pk_simplex_basis_vector_function[j];

    Vector_Function<Polynomial<space_dimension_>, This_::num_basis_> simplex_Pn_projection_basis_vector_function = simplex_Pn_projection_basis_functions;
    return simplex_Pn_projection_basis_vector_function;
}


template <ushort space_dimension_, ushort solution_order_>
bool hMLP_BD_Reconstruction<space_dimension_, solution_order_>::is_typeI_subcell_oscillation(const ushort num_trouble_boundaries) const {
    constexpr ushort typeI_threshold_value = 2;
    return typeI_threshold_value <= num_trouble_boundaries;
}

template <ushort space_dimension_, ushort solution_order_>
bool hMLP_BD_Reconstruction<space_dimension_, solution_order_>::is_typeII_subcell_oscillation(const ushort num_trouble_boundaries) const {
    constexpr ushort typeI_threshold_value = 1;
    return typeI_threshold_value <= num_trouble_boundaries;
}