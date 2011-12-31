#include "definitions.h"

CustomWeakFormMagnetostatics::CustomWeakFormMagnetostatics(std::string material_iron_1, std::string material_iron_2,
  CubicSpline* mu_inv_iron, std::string material_air,
  std::string material_copper, double mu_vacuum,
  double current_density, int order_inc) : WeakForm<double>(1) 
{

  // Jacobian.
  add_matrix_form(new DefaultJacobianMagnetostatics<double>(0, 0, Hermes::vector<std::string>(material_air, material_copper),
    1.0, HERMES_DEFAULT_SPLINE, HERMES_NONSYM, HERMES_AXISYM_Y, order_inc));
  add_matrix_form(new DefaultJacobianMagnetostatics<double>(0, 0, Hermes::vector<std::string>(material_iron_1, material_iron_2), 1.0,
    mu_inv_iron, HERMES_NONSYM, HERMES_AXISYM_Y, order_inc));
  // Residual.
  add_vector_form(new DefaultResidualMagnetostatics<double>(0, Hermes::vector<std::string>(material_air, material_copper),
    1.0, HERMES_ONE, HERMES_AXISYM_Y, order_inc));
  add_vector_form(new DefaultResidualMagnetostatics<double>(0, Hermes::vector<std::string>(material_iron_1, material_iron_2),1.0, 
    mu_inv_iron, HERMES_AXISYM_Y, order_inc));
  add_vector_form(new DefaultVectorFormVol<double>(0, material_copper, new Hermes2DFunction<double>(-current_density * mu_vacuum)));
}


void FilterVectorPotential::filter_fn(int n, Hermes::vector<double*> values, double* result, Geom<double> *e)
{
  for (int i = 0; i < n; i++)
  {
    result[i] = 0;
    for(unsigned int j = 0; j < values.size(); j++)
      result[i] += sqr(values[j][i]);

    result[i] = std::sqrt(result[i]) * e->x[i];
  }
}

FilterVectorPotential::FilterVectorPotential(Hermes::vector<MeshFunction<double>*> solutions, Hermes::vector<int> items) 
  : MagFilter<double>(solutions, items) 
{
}

FilterFluxDensity::FilterFluxDensity(Hermes::vector<MeshFunction<double>*> solutions)
  : Filter<double>(solutions) 
{
}

double FilterFluxDensity::get_pt_value(double x, double y, int item) 
{
  error("Not implemented yet"); return 0;
}

void FilterFluxDensity::precalculate(int order, int mask)
{
  Quad2D* quad = quads[cur_quad];
  int np = quad->get_num_points(order, this->get_active_element()->get_mode());
  Node* node = new_node(H2D_FN_DEFAULT, np);

  sln[0]->set_quad_order(order, H2D_FN_VAL | H2D_FN_DX | H2D_FN_DY);
  sln[1]->set_quad_order(order, H2D_FN_VAL | H2D_FN_DX | H2D_FN_DY);

  double *dudx1, *dudy1, *dudx2, *dudy2;
  sln[0]->get_dx_dy_values(dudx1, dudy1);
  sln[1]->get_dx_dy_values(dudx2, dudy2);
  double *uval1 = sln[0]->get_fn_values();
  double *uval2 = sln[1]->get_fn_values();
  update_refmap();
  double *x = refmap->get_phys_x(order);

  for (int i = 0; i < np; i++)
  {
    node->values[0][0][i] = std::sqrt(sqr(dudy1[i]) + sqr(dudy2[i]) +
      sqr(dudx1[i] + ((x[i] > 1e-10) ? uval1[i] / x[i] : 0.0)) +
      sqr(dudx2[i] + ((x[i] > 1e-10) ? uval2[i] / x[i] : 0.0)));
  }

  if(nodes->present(order)) 
  {
    assert(nodes->get(order) == cur_node);
    ::free(nodes->get(order));
  }
  nodes->add(node, order);
  cur_node = node;
}

