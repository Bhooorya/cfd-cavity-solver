#include <vector>
#include <cmath>
#include <iostream>
#include <emscripten/bind.h>

using namespace emscripten;

class CavitySolver {
private:
    int Nx, Ny;
    double Re, dx, dy, omega_s, aspect_ratio;
    std::vector<double> psi, omega, u, v, p;

    int idx(int i, int j) const { return i + j * Nx; }

public:
    CavitySolver(int grid_size, double aspect, double reynolds, double sor_factor) {
        initGrid(grid_size, aspect, reynolds, sor_factor);
    }

    void initGrid(int grid_size, double aspect, double reynolds, double sor_factor) {
        Nx = grid_size;
        aspect_ratio = aspect;
        Ny = std::max(5, (int)std::round(grid_size * aspect_ratio)); 
        Re = reynolds;
        omega_s = sor_factor;

        dx = 1.0 / (Nx - 1);
        dy = aspect_ratio / (Ny - 1);

        int total_nodes = Nx * Ny;
        psi.assign(total_nodes, 0.0);
        omega.assign(total_nodes, 0.0);
        u.assign(total_nodes, 0.0);
        v.assign(total_nodes, 0.0);
        p.assign(total_nodes, 0.0);
    }

    // Now returns the maximum error so Javascript knows when to stop
    double step(int iterations) {
        double max_err = 0.0;
        for(int k = 0; k < iterations; k++) {
            solvePoissonSOR();
            applyBoundaryConditions();
            max_err = updateVorticityTransport();
        }
        calculateVelocities();
        return max_err;
    }

    void solvePoissonSOR() {
        double dx2 = dx * dx;
        double dy2 = dy * dy;
        double factor = (dx2 * dy2) / (2.0 * (dx2 + dy2));

        for (int j = 1; j < Ny - 1; j++) {
            for (int i = 1; i < Nx - 1; i++) {
                double psi_star = factor * (
                    (psi[idx(i+1, j)] + psi[idx(i-1, j)]) / dx2 +
                    (psi[idx(i, j+1)] + psi[idx(i, j-1)]) / dy2 +
                    omega[idx(i, j)]
                );
                psi[idx(i, j)] = (1.0 - omega_s) * psi[idx(i, j)] + omega_s * psi_star;
            }
        }
    }

    void applyBoundaryConditions() {
        double dx2 = dx * dx;
        double dy2 = dy * dy;
        double U_lid = 1.0; 

        for (int i = 0; i < Nx; i++) {
            omega[idx(i, Ny-1)] = -2.0 * (psi[idx(i, Ny-2)] - psi[idx(i, Ny-1)] + U_lid * dy) / dy2;
            psi[idx(i, Ny-1)] = 0.0;
        }
        for (int i = 0; i < Nx; i++) {
            omega[idx(i, 0)] = -2.0 * (psi[idx(i, 1)] - psi[idx(i, 0)]) / dy2;
            psi[idx(i, 0)] = 0.0;
        }
        for (int j = 0; j < Ny; j++) {
            omega[idx(0, j)] = -2.0 * (psi[idx(1, j)] - psi[idx(0, j)]) / dx2;
            psi[idx(0, j)] = 0.0;
            omega[idx(Nx-1, j)] = -2.0 * (psi[idx(Nx-2, j)] - psi[idx(Nx-1, j)]) / dx2;
            psi[idx(Nx-1, j)] = 0.0;
        }
    }

    double updateVorticityTransport() {
        std::vector<double> omega_new = omega;
        double max_err = 0.0;
        
        for (int j = 1; j < Ny - 1; j++) {
            for (int i = 1; i < Nx - 1; i++) {
                double diff_x = (omega[idx(i+1, j)] - 2.0*omega[idx(i, j)] + omega[idx(i-1, j)]) / (dx * dx);
                double diff_y = (omega[idx(i, j+1)] - 2.0*omega[idx(i, j)] + omega[idx(i, j-1)]) / (dy * dy);
                double diffusion = (1.0 / Re) * (diff_x + diff_y);

                // Upwind Scheme for stability
                double u_vel = (psi[idx(i, j+1)] - psi[idx(i, j-1)]) / (2.0 * dy);
                double v_vel = -(psi[idx(i+1, j)] - psi[idx(i-1, j)]) / (2.0 * dx);
                
                double conv_x = (u_vel > 0) ? u_vel * (omega[idx(i, j)] - omega[idx(i-1, j)]) / dx 
                                            : u_vel * (omega[idx(i+1, j)] - omega[idx(i, j)]) / dx;
                double conv_y = (v_vel > 0) ? v_vel * (omega[idx(i, j)] - omega[idx(i, j-1)]) / dy 
                                            : v_vel * (omega[idx(i, j+1)] - omega[idx(i, j)]) / dy;
                
                double dt = 0.001; 
                omega_new[idx(i, j)] = omega[idx(i, j)] + dt * (diffusion - conv_x - conv_y);
                
                // Track Convergence
                double err = std::abs(omega_new[idx(i, j)] - omega[idx(i, j)]);
                if (err > max_err) max_err = err;
            }
        }
        omega = omega_new;
        return max_err;
    }

    void calculateVelocities() {
        for (int j = 1; j < Ny - 1; j++) {
            for (int i = 1; i < Nx - 1; i++) {
                u[idx(i, j)] = (psi[idx(i, j+1)] - psi[idx(i, j-1)]) / (2.0 * dy);
                v[idx(i, j)] = -(psi[idx(i+1, j)] - psi[idx(i-1, j)]) / (2.0 * dx);
            }
        }
        // Lid velocity
        for (int i = 0; i < Nx; i++) { u[idx(i, Ny-1)] = 1.0; v[idx(i, Ny-1)] = 0.0; }
    }

    void solvePressureIterations(int iterations) {
        double dx2 = dx * dx;
        double dy2 = dy * dy;
        double factor = (dx2 * dy2) / (2.0 * (dx2 + dy2));

        for(int k = 0; k < iterations; k++) {
            for(int j = 1; j < Ny - 1; j++) {
                for(int i = 1; i < Nx - 1; i++) {
                    double dudx = (u[idx(i+1, j)] - u[idx(i-1, j)]) / (2.0*dx);
                    double dudy = (u[idx(i, j+1)] - u[idx(i, j-1)]) / (2.0*dy);
                    double dvdx = (v[idx(i+1, j)] - v[idx(i-1, j)]) / (2.0*dx);
                    double dvdy = (v[idx(i, j+1)] - v[idx(i, j-1)]) / (2.0*dy);

                    double S = 2.0 * (dudx * dvdy - dudy * dvdx);

                    double p_star = factor * (
                        (p[idx(i+1, j)] + p[idx(i-1, j)]) / dx2 +
                        (p[idx(i, j+1)] + p[idx(i, j-1)]) / dy2 - S
                    );
                    p[idx(i,j)] = (1.0 - 1.7)*p[idx(i,j)] + 1.7*p_star;
                }
            }

            // Neumann Boundary Conditions for Pressure (dp/dn = 0)
            for(int i = 0; i < Nx; i++) { p[idx(i, 0)] = p[idx(i, 1)]; p[idx(i, Ny-1)] = p[idx(i, Ny-2)]; }
            for(int j = 0; j < Ny; j++) { p[idx(0, j)] = p[idx(1, j)]; p[idx(Nx-1, j)] = p[idx(Nx-2, j)]; }

            p[idx(Nx/2, Ny/2)] = 0.0; // Pin center pressure to prevent floating
        }
    }

    // Solves for pressure only once at the end (steady state)
    void solvePressure() {
        solvePressureIterations(3000);
    }

    val getPsi() { return val(typed_memory_view(psi.size(), psi.data())); }
    val getOmega() { return val(typed_memory_view(omega.size(), omega.data())); }
    val getU() { return val(typed_memory_view(u.size(), u.data())); }
    val getV() { return val(typed_memory_view(v.size(), v.data())); }
    val getP() { return val(typed_memory_view(p.size(), p.data())); }
    int getNx() { return Nx; }
    int getNy() { return Ny; }
};

EMSCRIPTEN_BINDINGS(cavity_module) {
    class_<CavitySolver>("CavitySolver")
        .constructor<int, double, double, double>()
        .function("initGrid", &CavitySolver::initGrid)
        .function("step", &CavitySolver::step)
        .function("solvePressureIterations", &CavitySolver::solvePressureIterations)
        .function("solvePressure", &CavitySolver::solvePressure)
        .function("getPsi", &CavitySolver::getPsi)
        .function("getOmega", &CavitySolver::getOmega)
        .function("getU", &CavitySolver::getU)
        .function("getV", &CavitySolver::getV)
        .function("getP", &CavitySolver::getP)
        .function("getNx", &CavitySolver::getNx)
        .function("getNy", &CavitySolver::getNy);
}
