#include "energy_solver.h"


IsingModel::IsingModel(int spin_mat_dim, int mc_iterations_input, double seed)
    : uniform_discrete(0, spin_mat_dim - 1), engine(seed),
    uniform_continuous(0, 1), spin(spin_mat_dim, seed)
{   /*
    Parameters
    ----------
    spin_mat_dim : int
        The spin matrix is of dimenstion spin_mat_dim x spin_mat_dim.

    mc_iterations_input : int
        The number of Monte Carlo iterations.

    seed : long
        Seed for the PRNG.
    */
    
    n = spin_mat_dim;
    mc_iterations = mc_iterations_input;
    total_energy_and_magnetization(spin, n, total_energy, total_magnetization);
    
    // initialising data files
    // E_convergence_data.open("data_files/E_convergence_data.txt", std::ios_base::app);
    // M_convergence_data.open("data_files/M_convergence_data.txt", std::ios_base::app);
    // ising_model_data.open("data_files/ising_model_data.txt", std::ios_base::app);
}


void IsingModel::mc_iteration_convergence(double temp)
{   /*
    Runs the spin flip a given amount of times. Generates data for finding
    how many iterations is needed for convergence. Keeps the energy values
    without averaging.

    Parameters
    ----------
    temp : double
        Temperature value.
    */

    for (int j = 0; j < mc_iterations; j++)
    {   // loops over n*n spin flips a given amount of times
        // saves relevant data for each iteration

        iterate_spin_flip(temp);
        E_convergence_data << std::setw(15) << total_energy;
        M_convergence_data << std::setw(15) << total_magnetization;
    }
}


void IsingModel::mc_iteration_stable(double temp)
{   /*
    Run the spin flip a given amount of times. Calculate the average
    values instead of keeping all data.

    Parameters
    ----------
    temp : double
        Temperature value.
    */

    sum_total_energy = 0;
    sum_total_energy_squared = 0;
    sum_total_magnetization  = 0;
    sum_total_magnetization_absolute = 0;
    sum_total_magnetization_squared  = 0;

    // int stable_iterations = 1e6;

    int i;
    for (i = 0; i < stable_iterations; i++)
    {   /*
        Runs the spin flip until the system is stable. Separate loop
        to avoid an if statement. This saves us computation time
        since we aren't interested in calculating any vaues in the
        unstable phase.
        */
        
        iterate_spin_flip(temp);
    }

    for (int j = i; j < mc_iterations; j++)
    {   // loops over n*n spin flips a given amount of times

        iterate_spin_flip(temp);

        sum_total_energy += total_energy;
        sum_total_energy_squared += total_energy*total_energy;
        sum_total_magnetization  += total_magnetization;
        sum_total_magnetization_absolute += std::fabs(total_magnetization);
        sum_total_magnetization_squared  += total_magnetization*total_magnetization;

    }
    
    sum_total_energy /= mc_iterations - stable_iterations;
    sum_total_energy_squared /= mc_iterations - stable_iterations;
    sum_total_magnetization  /= mc_iterations - stable_iterations;
    sum_total_magnetization_absolute /= mc_iterations - stable_iterations;
    sum_total_magnetization_squared  /= mc_iterations - stable_iterations;

}


void IsingModel::iterate_spin_flip(double temp)
{   /*
    Pick a random row and a random column. Pick a random number for the
    Metropolis condition. Flip the spin with the function
    metropolis_flap n*n times.

    Parameters
    ----------
    temp : double
        Temperature value for which to calculate the data.
    */
    
    for (int i = 0; i < n*n; i++)
    {   // flips n*n randomly drawn spins in the spin matrix
        
        metropolis_flap(spin, total_energy, total_magnetization,
            uniform_discrete(engine), uniform_discrete(engine),
            uniform_continuous(engine), temp, exp_delta_energy);
    }
}


void IsingModel::metropolis_flap(CircularMatrix& spin, double& total_energy,
    double& total_magnetization, int row, int col, double metropolis_random,
    double temperature, double* exp_delta_energy)
{   /*
    Flips a spin and calculates the energy difference. Accepts/rejects
    the flip based on the Metropolis algorithm.

    
    Parameters
    ----------
    spin : CircularMatrix
        Matrix of spin values.
    
    total_energy : double reference
        Total energy of all the spins.

    total_magnetization : double reference
        Total magnetic moment of the system.

    row : int
        A randomly chosen row index.

    col : int
        A randomly chosen column index.

    metropolis_random : double
        Random variable drawn from a uniform distribution on the
        interval [0, 1). Metropolis condition.

    temperature : double
        Temperature of the system.

    exp_delta_energy : double pointer
        Array containing the exponential of the possible energies.
    
    Note
    ----
    spin_here  = spin[row][col]
    */

    spin_here = spin(row, col);
    delta_energy = 2*spin_here*(spin(row-1, col) + spin(row+1, col)
        + spin(row, col+1) + spin(row, col-1));
    
    if (metropolis_random <= exp_delta_energy[(int) (delta_energy + 8)])
    {   // checks if energy difference is positive and the metropolis
        // condition true
        accepted_config++;  // Count the number of accepted configurations.
        spin(row, col)      *= -1;
        total_energy        += delta_energy;
        total_magnetization += -2*spin_here;
    }
}


void IsingModel::iterate_temperature(double initial_temp, double final_temp,
    double dtemp, bool convergence)
{   /*
    Iterate over a given set of temperature values.

    Parameters
    ----------
    initial_temp : double
        Start temperature value.

    final_temp : double
        End temperature value.

    dtemp : double
        Temperature step length.

    convergence : bool
        This class generates data, either for the convergence phase or
        for the stable phase. Convergence toggles this.
    */



    if (convergence)
    {   // title for the convergence files

        if (not is_conv_filename_set)
        {
            set_convergence_filenames();
            is_conv_filename_set = true;
        }

        E_convergence_data << "mc_iterations: " << mc_iterations;
        E_convergence_data << " spin_matrix_dim: " << n;
        E_convergence_data << std::endl;
        M_convergence_data << "mc_iterations: " << mc_iterations;
        M_convergence_data << " spin_matrix_dim: " << n;
        M_convergence_data << std::endl;
    }
    else
    {   // title for the stable file

        if (not is_ising_filename_set)
        {
            set_ising_filename();
            is_ising_filename_set = true;
        }

        ising_model_data << "mc_iterations: " << mc_iterations;
        ising_model_data << " spin_matrix_dim: " << n;
        ising_model_data << std::endl;
        ising_model_data << std::setw(20) << "T";
        ising_model_data << std::setw(20) << "<E>";
        ising_model_data << std::setw(20) << "<E**2>";
        ising_model_data << std::setw(20) << "<M>";
        ising_model_data << std::setw(20) << "<M**2>";
        ising_model_data << std::setw(20) << "<|M|>";
        ising_model_data << std::endl;
    }
    
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

    for (double temp = initial_temp; temp <= final_temp; temp += dtemp)
    {   // looping over temperature values
        // pre-calculated exponential values
        
        
        std::cout << "temp: " << temp << " of: " << final_temp;
        std::cout << " dtemp: " << dtemp << std::endl;
        
        
        exp_delta_energy[0]  = std::exp(8*J/temp);
        exp_delta_energy[4]  = std::exp(4*J/temp);
        exp_delta_energy[8]  = 1;
        exp_delta_energy[12] = std::exp(-4*J/temp);
        exp_delta_energy[16] = std::exp(-8*J/temp);

        if (convergence)
        {   // generates and writes convergence data                
            // writing temperature values in the first column
            E_convergence_data << std::setw(15) << temp;
            M_convergence_data << std::setw(15) << temp;
            
            mc_iteration_convergence(temp);
            
            E_convergence_data << std::endl;
            M_convergence_data << std::endl;
        }
        else
        {   // generates and writes stable data
            mc_iteration_stable(temp);
            ising_model_data << std::setw(20) << std::setprecision(15) << temp;
            ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_energy;
            ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_energy_squared;
            ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_magnetization;
            ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_magnetization_squared;
            ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_magnetization_absolute;
            ising_model_data << std::endl;
        }

        // ending timer
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> comp_time  = std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1);
        std::cout << "time since beginning: " << comp_time.count() << std::endl;
        std::cout << std::endl;
    }
}


void IsingModel::iterate_monte_carlo_cycles(int initial_MC, int final_MC, int dMC)
{   /*
    Loop over different number of Monte Carlo iterations. Currently
    only implemented for the stable phase.

    Parameters
    ----------
    initial_MC : int
        The initial number of Monte Carlo iterations.

    final_MC : int
        The final number of Monte Carlo iterations.

    dMC : int
        Monte Carlo iterations step length.
    */

    if (not is_ising_filename_set)
    {
       set_ising_filename();
       is_ising_filename_set = true;
    }

    // File title.
    ising_model_data << "initial_MC: " << initial_MC;
    ising_model_data << " final_MC : " << final_MC;
    ising_model_data << " dMC: " << dMC;
    ising_model_data << " spin_matrix_dim: " << n;
    ising_model_data << std::endl;
    ising_model_data << std::setw(20) << "MC";
    ising_model_data << std::setw(20) << "<E>";
    ising_model_data << std::setw(20) << "<E**2>";
    ising_model_data << std::setw(20) << "<M>";
    ising_model_data << std::setw(20) << "<M**2>";
    ising_model_data << std::setw(20) << "<|M|>";
    ising_model_data << std::endl;

    double temp = 1;

    exp_delta_energy[0]  = std::exp(8*J/temp);
    exp_delta_energy[4]  = std::exp(4*J/temp);
    exp_delta_energy[8]  = 1;
    exp_delta_energy[12] = std::exp(-4*J/temp);
    exp_delta_energy[16] = std::exp(-8*J/temp);

    for (int MC = initial_MC; MC < final_MC; MC += dMC)
    {   // Iterates over a set of Monte Carlo iterations.
        
        mc_iteration_stable(temp);
        ising_model_data << std::setw(20) << std::setprecision(15) << MC;
        ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_energy;
        ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_energy_squared;
        ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_magnetization;
        ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_magnetization_squared;
        ising_model_data << std::setw(20) << std::setprecision(15) << sum_total_magnetization_absolute;
        ising_model_data << std::endl;
    }
}


void IsingModel::total_energy_and_magnetization(CircularMatrix& spin, int n,
    double& total_energy, double& total_magnetization)
{   /*
    Compute from upper left in matrix and down to the right.

    Parameters
    ----------
    spin : CircularMatrix
        n x n matrix.
    
    n : int
        Grid dimension.

    total_energy : double reference
        Total energy.

    total_magnetization : double reference
        Total magnetic moment.
    */
    
    for (int i = 0; i < n; i++)
    {   // looping over rows 
        for (int j = 0; j < n; j++)
        {   /*
            looping over columns
            spin(i, j):     spin of the current indices
            spin(i+1, j):   spin below
            spin(i, j+1):   spin to the right
            */
            total_energy        -= spin(i, j)*(spin(i, j+1) + spin(i+1, j));
            total_magnetization += spin(i, j);
        }
    }
}


void IsingModel::set_new_input(int spin_mat_dim, int mc_iterations_input,
    double J_input, double seed)
{   /*
    Set all parameters to new input values.

    Parameters
    ----------
    spin_mat_dim : int
        Matrix dimension.

    mc_iterations_input : int
        Number of Monte Carlo iterations.

    J_input : double
        Strength of the interaction between neighbouring spins.

    seed : long
        Seed for the PRNG.
    */
    n = spin_mat_dim;
    mc_iterations = mc_iterations_input;
    J = J_input;

    engine.seed(seed);
    spin.set_new_dim_and_seed(spin_mat_dim, seed);
}


void IsingModel::set_interactions_strength(double J_input)
{   /*
    Set the strength of the spin interactions.

    Parameters
    ----------
    J_input : double
        Strength of the interaction between neighbouring spins.
    */
    
    J = J_input;
}


void IsingModel::set_mc_iterations(int mc_iterations_input)
{   /*
    Set the number of Monte Carlo iterations.

    Parameters
    ----------
    mc_iterations_input : int
        Number of Monte Carlo iterations.
    */

    mc_iterations = mc_iterations_input;
}


void IsingModel::set_stable_iterations(int stable_iterations_input)
{   /*
    Parameters
    ----------
    stable_iterations : int
        Number of mc iterations befor data is gathered/before system is stable.
    */

    stable_iterations = stable_iterations_input;
}


void IsingModel::set_spin_dim(int spin_mat_dim)
{   /*
    Set the dimention of the spin matrix.

    Parameters
    ----------
    spin_mat_dim : int
        Matrix dimension.
    */

    n = spin_mat_dim;
    spin.set_new_dim(spin_mat_dim);
}


void IsingModel::set_order_spins()
{   /*
    Set the spin matrix to ordered initial configuration.
    */
    spin.ordered_spin();
}


void IsingModel::set_convergence_filenames()
{   /*
    Sets pre-defined filenames.
    */

    E_convergence_data.open("data_files/E_convergence_data.txt", std::ios_base::app);
    M_convergence_data.open("data_files/M_convergence_data.txt", std::ios_base::app);
}


void IsingModel::set_convergence_filenames(std::string postfix)
{   /*
    Set the filenames of convergence files.

    Parameters
    ----------
    postfix : std::string
        Addition to end of filename of pre-set filenames.
    */

    std::string filename1 = "data_files/E_convergence_data_" + postfix + ".txt";
    std::string filename2 = "data_files/M_convergence_data_" + postfix + ".txt";
    E_convergence_data.open(filename1, std::ios_base::app);
    M_convergence_data.open(filename2, std::ios_base::app);
    is_conv_filename_set = true;
}


void IsingModel::set_ising_filename()
{   /*
    Sets pre-defined filename.
    */

    ising_model_data.open("data_files/ising_model_data.txt", std::ios_base::app);
}


void IsingModel::set_ising_filename(std::string postfix)
{   /*
    Set the filenames of convergence files.

    Parameters
    ----------
    postfix : std::string
        Addition to end of filename of pre-set filenames.
    */

    std::string filename = "data_files/ising_model_data_" + postfix + ".txt";
    ising_model_data.open(filename, std::ios_base::app);
    is_ising_filename_set = true;
}


IsingModel::~IsingModel()
{
    delete[] exp_delta_energy;
}
