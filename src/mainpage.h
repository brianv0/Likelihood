// Mainpage for doxygen

/** @mainpage package Likelihood

 @authors James Chiang, Pat Nolan, Karl Young, and others

 @section intro Introduction

 This package implements an extended maximum likelihood (EML)
 calculation for analyzing LAT event data.

 These data are assumed to be in a format consistent with that
 produced by the Level 1 Event Data Extractor, otherwise known as U1.
 The data may alternatively have been generated by the observation
 simulator (O2).  Accordingly, use of this tool for analysis of Event
 data requires access to a complete set of accompanying spacecraft
 orbit and attitude information, obtained using the Pointing, Livetime
 History Extractor (U3) or the orbit simulator tool (O1), as well as
 access to appropriate instrument response function data (i.e.,
 CALDB).

 However, the classes and methods used here are intended to be
 sufficiently general so that any properly implemented objective
 function should be able to be analyzed with this package, whether it
 is LAT-specific or not.

 @section LatStatModel The Unbinned log-Likelihood

 For LAT event analysis, the default statistical model we assume is
 the unbinned log-likelihood:

 \f[
 \log L = \sum_j \left[\log \left(\sum_i M_i(x_j; \vec{\alpha_i})\right)\right] 
        - \sum_i \left[\int dx M_i(x; \vec{\alpha_i})\right]
 \f]

 Here \f$x_j\f$ is the \f$j\f$th photon Event, as specified by
 apparent energy, direction, and arrival time. The function \f$M_i(x;
 \vec{\alpha_i})\f$ returns the flux density in units of counts per
 energy-time-area-solid angle (i.e., photon fluxes convolved through
 the instrument response) for the \f$i\f$th Source at a point \f$x\f$
 in the Event configuration space, hereafter known as the "data
 space".  Each \f$M_i\f$ is defined, in part, by a vector of parameter
 values \f$\vec{\alpha_i}\f$; collectively, the \f$\vec{\alpha_i}\f$
 vectors form the space over which the objective function is to be
 optimized.  The integral over the data space in the second term is
 the predicted number of Events expected to be seen from Source
 \f$i\f$.

 @section classes Important Classes

 Cast in this form, the problem lends itself to being described by the
 following classes.  Some of these classes now reside in the optimizers
 and latResponse packages.

   - optimizers::Function Objects of this class act as "function
   objects" in that the function call operator () is overloaded so
   that instances behave like ordinary C functions.  Several methods
   are also provided for accessing the model Parameters and
   derivatives with respect to those Parameters, either singly or in
   groups.  The behavior of this class is greatly facilitated by the
   Parameter and Arg classes.

   - optimizers::Parameter This is essentially an n-tuple containing
   model parameter information (and access methods) comprising the
   parameter value, scale factor, name, upper and lower bounds and
   whether the parameter is to be considered free or fixed in the
   fitting process.

   - optimizers::Arg This class wraps arguments to Function objects so
   that Function's derivative passing mechanisms can be inherited by
   subclasses regardless of the actual type of the underlying
   argument.  For example, in the log-likelihood, we define
   Likelihood::logSrcModel, a Function subclass that returns the
   quantity inside the square brackets of the first term on the rhs.
   Acting as a function, logSrcModel naturally wants to take an Event
   as its argument, so we wrap an Event object with EventArg.
   Similarly, the quantity in the square brackets of the second term
   on the rhs we implement as the Likelihood::Npred class.  This class
   wants to have a Source object as its argument, which we wrap with
   the SrcArg class.

   - Likelihood::Source An abstract base class for gamma-ray sources.
   It specifies four key methods (as pure virtual functions); the
   latter two methods are wrapped by the Npred class in order to give
   them Function behavior:
      - fluxDensity(...): counts per energy-time-area-solid angle
      - fluxDensityDeriv(...): derivative wrt a Parameter
      - Npred(): predicted number of photons in the ROI
      - NpredDeriv(...): derivative of Npred wrt a Parameter

   - Likelihood::Event An n-tuple containing photon event arrival
   time, apparent energy and direction, as well as spacecraft attitude
   information at the event arrival time and event-specific response
   function data for use with components of the diffuse emission
   model.

   - latResponse::[IAeff, IPsf, IEdisp] These classes provide abstract
   interfaces to the instrument response functions comprising the
   effective area, the point-spread function, and the energy
   dispersion.  Concrete implementations exist for the Glast25
   parameterizations as well as for the EGRET response functions.

   - Likelihood::RoiCuts An n-tuple Singleton class that contains the
   "region-of-interest" cuts.  These are essentially the bounds of the
   data space as a function of arrival time, apparent energy, apparent
   direction, zenith angle, etc..

   - Likelihood::ScData A Singleton object that contains the
   spacecraft data n-tuples (ScNtuple).

   - Likelihood::SourceFactory This class provides a common access
   point for retrieving and storing Sources. Sources that have been
   constructed to comprise position and spectral information can be
   stored here, then cloned for later use.  The sources can also be
   read in from and output to an xml file.

   - optimizers::Optimizer An abstract base class for the algorithms
   which maximize the desired objective functions.  Choice of
   optimization methods are encapsulated in three sub-classes which
   simply wrap existing implementations that are/were originally
   available as Fortran code: Lbfgs, Minuit, and Drmngb.  

 <hr>
 @section notes Release Notes
  release.notes

 <hr>
 @section requirements requirements
 @verbinclude requirements

 <hr> 
 @todo Energy dispersion
 @todo Generalize Npred calculation, e.g., zenith angle cuts, fit-able 
       source locations
 @todo Refactor Statistic and FITS-related classes (Table, FitsImage, etc.)
 @todo Use more realistic response function data
 @todo Analyze EGRET data
 @todo Boost.Python
 */

/**
 @page userGuide User's Guide

 @section likeApp likelihoodApp

 This is an FTOOLS-like interface to the Likelihood class library.
 It uses HOOPS to obtain the command-line parameters and therefore
 requires a .par file called <a href="http://glast.stanford.edu/cgi-bin/cvsweb/Likelihood/data/likelihood.par?cvsroot=CVS_SLAC">likelihood.par</a>:

@verbinclude likelihood.par

 The format of the entries in a .par file is given in the <a
 href="http://www-glast.slac.stanford.edu/sciencetools/userInterface/doc/pil.pdf">PIL user
 manual</a>.  The ordering of the fields in each line are
 - variable name
 - variable type (s=string, r=real, i=integer, b=boolean)
 - query mode (q=ask, h=hidden, l=learn)
 - default value
 - lower bound or a list of allowed selections delimited by "|"
 - upper bound
 - prompt string

 The hidden parameters are not queried for, but they must be provided
 as an argument on the command line if quantities that follow it are
 given so that the ordering is preserved.

 Here is a sample session in which one day's worth of data simulated
 for the whole sky, including the 271 3EG sources, Galactic Diffuse
 emission, are analyzed.  The region-of-interest is defined around the
 Galactic anticenter.
 @verbatim
glast-guess1[jchiang] likelihoodApp.exe
ROI cuts file [RoiCuts.xml] : 
Spacecraft file [all_sky_1day_scData_0000.fits] : 
Exposure file [anticenter_expMap.fits] : 
Response functions to use <FRONT/BACK|COMBINED> [FRONT/BACK] : 
Source model file [anticenter_model.xml] : 
Event file [all_sky_1day_events_0000.fits] : 
Optimizer <LBFGS|MINUIT|DRMNGB> [MINUIT] : 
Optimizer verbosity [0] : 
Fit tolerance [0.0001] : 
Source model output file [anticenter_model.xml] : 
Use OptEM? [no] : 
flux-style output file name [anticenter_flux_model.xml] : 
Allow for refitting? [yes] : 
LogLike::getEvents:
Out of 129029 events in file all_sky_1day_events_0000.fits,
 10468 were accepted, and 118561 were rejected.

Creating source named Crab
Computing exposure at (83.57, 22.01).....................!
Creating source named Extragalactic Diffuse Emission
Creating source named Galactic Diffuse Emission
Creating source named Geminga
Computing exposure at (98.49, 17.86).....................!
Creating source named PKS0528p134
Computing exposure at (82.74, 13.38).....................!
adding source Crab
adding source Extragalactic Diffuse Emission
adding source Galactic Diffuse Emission
adding source Geminga
adding source PKS0528p134
Computing Event responses for the DiffuseSources.....................!
  MINUIT RELEASE 96.03  INITIALIZED.   DIMENSIONS 100/100  EPSMAC=  0.89E-15

Crab:
Prefactor: 46.0849 +/- 3.06622
Index: -2.37168 +/- 0.0517837
Scale: 100
Npred: 793.844

Extragalactic Diffuse Emission:
Prefactor: 0.383608 +/- 0.193267
Index: -2.15087 +/- 0.0888472
Scale: 100
Npred: 393.716

Galactic Diffuse Emission:
Prefactor: 14.3603 +/- 0.394709
Index: -2.10968 +/- 0.00986923
Scale: 100
Npred: 8527.21

Geminga:
Prefactor: 25.5869 +/- 2.35634
Index: -1.68864 +/- 0.0336242
Scale: 100
Npred: 501.452

PKS0528p134:
Prefactor: 15.5845 +/- 2.30596
Index: -2.35068 +/- 0.100206
Scale: 100
Npred: 243.092

Writing fitted model to anticenter_model.xml
Refit? [y] n
Writing flux-style xml model file to anticenter_flux_model.xml
glast-guess1[jchiang] 
 @endverbatim

Since this tool is largely non-interactive, control over the source
fitting is afforded by specifying the parameters appearing in the
.par file.  Each of the parameters will be discussed in turn.

 - @b ROI_cuts_file  The <a
   href="http://glast.stanford.edu/cgi-bin/cvsweb/Likelihood/xml/RoiCuts.xml?cvsroot=CVS_SLAC">ROI
   cuts file</a> contains basic information about the extraction
   region, valid time ranges, energies, etc.:
@verbatim
<?xml version='1.0' standalone='no'?>
<Region-of-Interest title="Anticenter Region">
   <timeInterval start="0"
                 stop="1"
                 unit="days"/>
   <energies emin="30." 
             emax="3.1623e5"
             unit="MeV"/>
   <acceptanceCone longitude="180."
                   latitude="0."
                   radius="25"
                   coordsys="Galactic"/>
</Region-of-Interest>
@endverbatim

 - @b Spacecraft_file The Spacecraft file is either a single FITS file
   or a list of FITS files given in correct simulation time order.
   Here's an example:
@verbatim
virgo_region_scData_0000.fits
virgo_region_scData_0001.fits
@endverbatim
   The code looks at the first 6 characters of whatever file is
   specified.  If they match the FITS keyword "SIMPLE", the file is
   assumed to be a FITS file.  If not, then it is assumed to be a list
   of FITS files.  Note that FITS files may be easily concatenated
   using the @b fmerge FTOOL.

 - @b Spacecraft_file_hdu This should always the second HDU in the file,
   but access to this value is given for flexibility's sake.

 - @b Exposure_map_file If there are diffuse components in the source
   model, then one must specify an exposure map that has been computed
   using the @ref expMap application.  If the data contain only point
   sources, then the exposure map can be omitted.

 - @b Response_functions Presently, only Front/Back and Combined
   response functions for the Glast25 parameterizations are available.
   When additional LAT IRFs are available, they will be included in
   the list of valid options.

 - @b Source_model_file The Source model file contains an xml
   description of the various sources to be modeled.  Here's the
   source model file used in the above fit:
@verbatim
<source_library title="source library">
  <source name="Crab" type="PointSource">
    <spectrum type="PowerLaw">
      <parameter max="1000" min="0.001" free="1" name="Prefactor" scale="1e-09" value="46.0849" />
      <parameter max="-1" min="-3.5" free="1" name="Index" scale="1" value="-2.37168" />
      <parameter max="200" min="50" free="0" name="Scale" scale="1" value="100" />
    </spectrum>
    <spatialModel type="SkyDirFunction">
      <parameter max="3.40282e+38" min="-3.40282e+38" free="0" name="RA" scale="1" value="83.57" />
      <parameter max="3.40282e+38" min="-3.40282e+38" free="0" name="DEC" scale="1" value="22.01" />
    </spatialModel>
  </source>
  <source name="Extragalactic Diffuse Emission" type="DiffuseSource">
    <spectrum type="PowerLaw">
      <parameter max="100" min="1e-05" free="1" name="Prefactor" scale="1e-07" value="0.383608" />
      <parameter max="-1" min="-3.5" free="1" name="Index" scale="1" value="-2.15087" />
      <parameter max="200" min="50" free="0" name="Scale" scale="1" value="100" />
    </spectrum>
    <spatialModel type="ConstantValue">
      <parameter max="10" min="0" free="0" name="Value" scale="1" value="1" />
    </spatialModel>
  </source>
  <source name="Galactic Diffuse Emission" type="DiffuseSource">
    <spectrum type="PowerLaw">
      <parameter max="1000" min="0.001" free="1" name="Prefactor" scale="0.001" value="14.3603" />
      <parameter max="-1" min="-3.5" free="1" name="Index" scale="1" value="-2.10968" />
      <parameter max="200" min="50" free="0" name="Scale" scale="1" value="100" />
    </spectrum>
    <spatialModel file="/u1/jchiang/SciTools/dev/Likelihood/v2r3/src/test/Data/gas.cel" type="SpatialMap">
      <parameter max="1000" min="0.001" free="0" name="Prefactor" scale="1" value="1" />
    </spatialModel>
  </source>
  <source name="Geminga" type="PointSource">
    <spectrum type="PowerLaw">
      <parameter max="1000" min="0.001" free="1" name="Prefactor" scale="1e-09" value="25.5869" />
      <parameter max="-1" min="-3.5" free="1" name="Index" scale="1" value="-1.68864" />
      <parameter max="200" min="50" free="0" name="Scale" scale="1" value="100" />
    </spectrum>
    <spatialModel type="SkyDirFunction">
      <parameter max="3.40282e+38" min="-3.40282e+38" free="0" name="RA" scale="1" value="98.49" />
      <parameter max="3.40282e+38" min="-3.40282e+38" free="0" name="DEC" scale="1" value="17.86" />
    </spatialModel>
  </source>
  <source name="PKS0528p134" type="PointSource">
    <spectrum type="PowerLaw">
      <parameter max="1000" min="0.001" free="1" name="Prefactor" scale="1e-09" value="15.5845" />
      <parameter max="-1" min="-3.5" free="1" name="Index" scale="1" value="-2.35068" />
      <parameter max="200" min="50" free="0" name="Scale" scale="1" value="100" />
    </spectrum>
    <spatialModel type="SkyDirFunction">
      <parameter max="3.40282e+38" min="-3.40282e+38" free="0" name="RA" scale="1" value="82.74" />
      <parameter max="3.40282e+38" min="-3.40282e+38" free="0" name="DEC" scale="1" value="13.38" />
    </spatialModel>
  </source>
</source_library>
@endverbatim

 - @b event_file The event file also can either be a single FITS file
   or a list.

 - @b event_file_hdu This should always be the second HDU in the event
   file.

 - @b optimizer The <a
   href="httphttp://www.slac.stanford.edu/exp/glast/ground/software/RM/documentation/ScienceTools/ScienceTools-v0r4p3/optimizers/v0r3/">optimizers</a>
   package provides three optimizers from which to choose, Lbfgs,
   Minuit, and Drmngb.  All three methods use a variant of the class
   of Variable Metric or Quasi-Newton methods for performing the
   optimizations.\n\n
   Drmngb seems to be the fastest and most robust of the three.
   Minuit provides error estimates using its internal calculation of
   the Hessian matrix, while for Lbfgs and Drmngb we have implemented
   a finte-difference estimate of the Hessian from which the
   covariance matrix is obtained.

 - @b fit_verbosity Entering "0" will result in @i almost no feedback
   from the optimizers being produced.  The larger the value, the more
   feedback from the various optimizers is produced.   
   
 - @b fit_tolerance This is the relative change in the negative
   log-likelihood value between successive estimates at which the
   optimization is considered to have converged.

 - @b Source_model_output_file After fitting, the source model can be
   written to an output file.  If the same file name as the input
   source model file is given, then the input file will be
   overwritten. If "none", then the data are not written.

 - @b Use_OptEM Likelihood::OptEM is an alternative method for
   performing the optimization using the expectation maximization
   method. (This could be made an option under the optimizer
   parameter, except that its internal optimizer --- Lbfgs, Drmngb,
   Minuit --- could, in principle, be chosen there.)

 - @b flux_style_model_file The source model can be written out as a
   flux-package style xml file that's suitable for use with either <a
   href="http://www.slac.stanford.edu/~jchiang/O2/doxy-html/">observationSim</a>
   or <a
   href="http://www.slac.stanford.edu/exp/glast/ground/software/RM/documentation/GlastRelease/GlastRelease-v3r3p7/Gleam/v5r5/">Gleam</a>.\n\n
Here's the resulting flux-style xml file for the above fit:
@verbatim
<source_library title="Likelihood_model">
  <source flux="0.17545" name="Crab">
    <spectrum escale="MeV">
      <particle name="gamma">
        <power_law emax="316230" emin="30" gamma="2.37168" />
      </particle>
      <celestial_dir ra="83.57" dec="22.01" />
    </spectrum>
  </source>
  <source flux="0.133398" name="Extragalactic_Diffuse_Emission">
    <spectrum escale="MeV">
      <particle name="gamma">
        <power_law emax="316230" emin="30" gamma="2.15087" />
      </particle>
      <solid_angle maxcos="1.0" mincos="-0.4" />
    </spectrum>
  </source>
  <source flux="49282.7" name="Galactic_Diffuse_Emission" />
  <source flux="0.0850587" name="Geminga">
    <spectrum escale="MeV">
      <particle name="gamma">
        <power_law emax="316230" emin="30" gamma="1.68864" />
      </particle>
      <celestial_dir ra="98.49" dec="17.86" />
    </spectrum>
  </source>
  <source flux="0.0587486" name="PKS0528p134">
    <spectrum escale="MeV">
      <particle name="gamma">
        <power_law emax="316230" emin="30" gamma="2.35068" />
      </particle>
      <celestial_dir ra="82.74" dec="13.38" />
    </spectrum>
  </source>
  <source name="all_in_anticenter_flux_model.xml">
    <nestedSource sourceRef="Crab" />
    <nestedSource sourceRef="Extragalactic_Diffuse_Emission" />
    <nestedSource sourceRef="Galactic_Diffuse_Emission" />
    <nestedSource sourceRef="Geminga" />
    <nestedSource sourceRef="PKS0528p134" />
  </source>
</source_library>
@endverbatim
For the flux-style sources, the name attributes cannot start with a
numeral and cannot have spaces, so underscores are added.  As a
convenience, a single composite source is created that comprises all
of the individual sources.  (NB: The flux for the Galactic Diffuse
Emission isn't being calculated correctly yet.)

 - @b query_for_refit If this is set to "yes", then the user is queried if
   he or she wishes to refit the data.  This allows the source model xml
   file to be modified by hand between fits, if, for example, one wishes
   to fix a parameter and/or set the value by hand.  Source positions can
   also be moved between fits (but they cannot be allowed to be free in
   the fit).\n\n
   In order to run @ref likelihood entirely non-interactively, as one
   would do in a script, this value must be set to "no".

 @section TsMap TsMap

In order to find sky locations of point sources, "test statistic" maps
are created.  These are computed by placing a putative point source at
each of the pixel locations in the map and then performing the fit by
maximizing the log-likelihood.  The test statistic value at that pixel
location is then given by
\f[
T_s = -2(\log L - \log L_0).
\f]

Here, \f$\log L\f$ is the maximum log-likelihood for the fit with the
putative point source and \f$\log L_0\f$ is the log-Likelihood in the
null hypothesis, i.e., a fit to the data without the putative point
source.  Note that there will likely be other source model components,
Galactic diffuse emission, other point sources, etc., in the source
model besides the putative point source.

Here is an example session of a TsMap run:

@verbatim
glast-guess1[jchiang] TsMap.exe 
ROI cuts file [$(LIKELIHOODROOT)/data/RoiCuts.xml] : 
Spacecraft file [$(LIKELIHOODROOT)/data/oneday_scData_0000.fits] : 
Exposure file [none] : 
Response functions to use <FRONT/BACK|FRONT|BACK> [FRONT/BACK] : 
Source model file [$(LIKELIHOODROOT)/data/anticenter_model.xml] : 
Event file [$(LIKELIHOODROOT)/data/oneday_events_0000.fits] : 
Optimizer <LBFGS|MINUIT|DRMNGB> [DRMNGB] : 
Optimizer verbosity [0] : 
Fit tolerance [0.001] : 
Coordinate system <CEL|GAL> [GAL] : 
Longitude minmum <-360 - 360> [160] : 
Longitude maximum <-360 - 360> [200] : 
Number of longitude points <2 - 200> [40] : 
Latitude minimum <-90 - 90> [-20] : 
Latitude maximum <-90 - 90> [20] : 
Number of latitude points <2 - 200> [40] : 
TS map file name [TsMap.fits] : 
Creating source named Crab
Computing exposure at (83.57, 22.01)......
@endverbatim

The first nine parameters are the same as for the @ref likeApp.  The new
ones describe the region of the sky to be mapped.  Here is 
an example file <a href="http://glast.stanford.edu/cgi-bin/cvsweb/Likelihood/data/TsMap.par?cvsroot=CVS_SLAC">TsMap.par</a>:

@verbinclude TsMap.par

 @section expMap expMap

This application creates an exposure map for use by the Likelihood
package.  Here's a sample session:

@verbatim
glast-guess1[jchiang] expMap.exe 
ROI cuts file [$(LIKELIHOODROOT)/data/RoiCuts.xml] : 
Spacecraft file [oneday_scData_0000.fits] : $(LIKELIHOODROOT)/data/oneday_scData_0000.fits
Response functions to use <FRONT/BACK|FRONT|BACK> [FRONT/BACK] : 
Radius of the source region (in degrees) [30] : 
Number of longitude points <2 - 1000> [60] : 
Number of latitude points <2 - 1000> [60] : 
Number of energies <2 - 100> [10] : 
Exposure file [anticenter_expMap.fits] : 
The radius of the source region, 30, should be significantly larger (say by 10 deg) than the ROI radius of 25
Computing the ExposureMap...................!
@endverbatim

Note that @b expMap uses the ROI cuts file, which should be the same
one used for the source analysis by @ref likeApp.  Contrary to
this example, one should choose sufficient resolution for the map,
meaning pixels that are typically less than 0.5 degrees on a side.
Also, since the broad PSFs of the instrument can include significant
numbers of photons from sources outside of the ROI, one should choose
a source region size that is significantly larger than the ROI.
Here's an example <a href="http://glast.stanford.edu/cgi-bin/cvsweb/Likelihood/data/expMap.par?cvsroot=CVS_SLAC">expMap.par</a> file.

*/
