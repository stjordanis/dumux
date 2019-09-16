This tutorial was copied from dumux/test/porousmediumflow/tracer/1ptracer.

# One-phase flow with random permeability distribution and a tracer model

## Problem set-up
This example contains a contaminant transported by a base groundwater flow in a randomly distributed permeability field. The figure below shows the simulation set-up. The permeability values range between 6.12e-15 and 1.5 e-7 $`m^2`$. A pressure gradient between the top an the bottom boundary leads to a groundwater flux from the bottom to the top. Neumann no-flow boundaries are assigned to the left and right boundary. Initially, there is a contaminant concentration at the bottom of the domain.

 <img src="img/setup.png" width="500">

## Model description
Two different models are applied to simulate the system: In a first step, the groundwater velocity is evaluated under stationary conditions. Therefore the single phase model is applied.
In a second step, the contaminant gets transported based on the groundwater velocity field. It is assumed, that the dissolved contaminant does not affect density and viscosity of the groundwater and thus, it is handled as a tracer by the tracer model. The tracer model is then solved instationarily.

### 1p Model
The single phase model uses Darcy's law as the equation for the momentum conservation:

$` \textbf v = - \frac{\textbf K}{\mu} \left(\textbf{grad}\, p - \varrho {\textbf g} \right) `$

With the darcy velocity $` \textbf v `$, the permeability $` \textbf K`$, the dynamic viscosity $` \mu`$, the pressure $`p`$, the density $`\rho`$ and the gravity $`\textbf g`$.

Darcy's law is inserted into the continuity equation:

$` \phi \frac{\partial \varrho}{\partial t} + \text{div} \textbf v = 0`$

with porosity $`\phi`$.

The equation is discretized using a cell-centered finite volume scheme as spatial discretization for the pressure as primary variable. For details on the discretization scheme, have a look at the dumux handbook.

### Tracer Model
The transport of the contaminant component $`\kappa`$ is based on the previously evaluated velocity field $`\textbf v`$  with the help the following mass balance equation:

$` \phi \frac{ \partial X^\kappa}{\partial t} - \text{div} \left\lbrace X^\kappa {\textbf v}+ D^\kappa_\text{pm} \frac{M^\kappa}{M_\alpha} \textbf{grad} x^\kappa \right\rbrace = 0 `$

With the porosity $`\phi`$, the mass fraction of the contaminant component $`\kappa`$: $`X^\kappa`$, the binary diffusion coefficient in the porous medium $` D^\kappa_\text{pm} `$, the molar masses of the component $` M^\kappa `$, the average molar mass of the phase $`M_\alpha`$ and the mole fraction $`x`$.

The porous medium diffusivity is yield out of the diffusion coefficient of the component, the porosity $`\phi `$ and the porous medium tortuosity $`\tau`$ by the following equation:

$`
D^\kappa_\text{pm}= \phi \tau D^\kappa
`$

The primary variable of this model is the mass fraction $`X^\kappa`$. We apply the same spatial discretization as in the single pahse model and use the implicit Euler method for time discretization. For more information, have a look at the dumux handbook.

In the following, we take a close look at the files containing the setup: At first, boundary conditions and spatially distributed parameters are set in `problem_1p.hh` and `spatialparams_1p.hh`, respectively, for the single phase model and subsequently in `problem_tracer.hh` and `spatialparams_tracer.hh` for the tracer model. Afterwards, we show the different steps for solving the model in the source file `main.cc`. At the end, we show some simulation results.