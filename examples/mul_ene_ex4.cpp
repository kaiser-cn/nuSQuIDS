 /******************************************************************************
 *    This program is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation, either version 3 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                             *
 *   Authors:                                                                  *
 *      Carlos Arguelles (University of Wisconsin Madison)                     *
 *         carguelles@icecube.wisc.edu                                         *
 *      Jordi Salvado (University of Wisconsin Madison)                        *
 *         jsalvado@icecube.wisc.edu                                           *
 *      Christopher Weaver (University of Wisconsin Madison)                   *
 *         chris.weaver@icecube.wisc.edu                                       *
 ******************************************************************************/



#include <vector>
#include <iostream>
#include "nuSQUIDS.h"

/*
 * This file demonstrates how use nuSQUIDS to propage neutrinos
 * considering oscillation as well as noncoherent interactions
 * such as CC/NC interactions and tau regeneration.
 */

using namespace nusquids;

class nuSQUIDSLV: public nuSQUIDS {
  private:
    squids::SU_vector LVP;
    std::vector<squids::SU_vector> LVP_evol;
  public:
    void AddToPreDerive(double x){
      for(int ei = 0; ei < ne; ei++){
        // asumming same mass hamiltonian for neutrinos/antineutrinos
        squids::SU_vector h0 = H0(E_range[ei],0);
        LVP_evol[ei] = LVP.Evolve(h0,(x-Get_t_initial()));
      }
    }

    squids:: SU_vector HI(unsigned int ei,unsigned int index_rho) const {
      return (1.0e-27*E_range[ei])*LVP_evol[ei];
    }

    nuSQUIDSLV(double Emin_,double Emax_,int Esize_,int numneu_,NeutrinoType NT_,
         bool elogscale_,bool iinteraction_)
    {
       Init(Emin_,Emax_,Esize_,numneu_,NT_,
           elogscale_,iinteraction_);
       // defining a complex matrix M which will contain our flavor
       // violating flavor structure.
       gsl_matrix_complex * M = gsl_matrix_complex_calloc(3,3);
       gsl_complex c {{ 1.0 , 0.0 }};
       gsl_matrix_complex_set(M,2,1,c);
       gsl_matrix_complex_set(M,1,2,gsl_complex_conjugate(c));

       LVP = squids::SU_vector(M);
       // rotate to mass reprentation
       LVP.RotateToB1(params);
       LVP_evol.resize(ne);
       for(int ei = 0; ei < ne; ei++){
         LVP_evol[ei] = squids::SU_vector(nsun);
       }
       gsl_matrix_complex_free(M);
    }
};

int main()
{
  nuSQUIDSLV nus(1.e4,1.e6,150,3,neutrino,true,false);

  double phi = acos(-1.);
  std::shared_ptr<EarthAtm> earth_atm = std::make_shared<EarthAtm>();
  std::shared_ptr<EarthAtm::Track> track_atm = std::make_shared<EarthAtm::Track>(phi);

  nus.Set_Body(earth_atm);
  nus.Set_Track(track_atm);

  // set mixing angles and masses
  nus.Set_MixingAngle(0,1,0.563942);
  nus.Set_MixingAngle(0,2,0.154085);
  nus.Set_MixingAngle(1,2,0.785398);

  nus.Set_SquareMassDifference(1,7.65e-05);
  nus.Set_SquareMassDifference(2,0.00247);

  // setup integration settings
  nus.Set_h_max( 100.0*nus.units.km );
  nus.Set_rel_error(1.0e-19);
  nus.Set_abs_error(1.0e-19);

  marray<double,1> E_range = nus.GetERange();

  // construct the initial state
  marray<double,2> inistate({150,3});
  double N0 = 1.0e18;
  for ( int i = 0 ; i < inistate.extent(0); i++){
      for ( int k = 0; k < inistate.extent(1); k ++){
        // initialze muon state
        inistate[i][k] = (k == 1) ? N0*pow(E_range[i],-1.0) : 0.0;
      }
  }

  // set the initial state
  nus.Set_initial_state(inistate,flavor);

  nus.Set_ProgressBar(true);
  nus.EvolveState();
  // we can save the current state in HDF5 format
  // for future use.
  nus.WriteStateHDF5("./mul_ene_ex4.hdf5");

  return 0;
}
