/*
  This file is part of MPTRAC.
  
  MPTRAC is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  MPTRAC is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with MPTRAC. If not, see <http://www.gnu.org/licenses/>.
  
  Copyright (C) 2013-2019 Forschungszentrum Juelich GmbH
*/

/*! 
  \file
  Extract vertical profile from meteorological data.
*/

#include "libtrac.h"

/* ------------------------------------------------------------
   Dimensions...
   ------------------------------------------------------------ */

/*! Maximum number of altitudes. */
#define NZ 1000

/* ------------------------------------------------------------
   Main...
   ------------------------------------------------------------ */

int main(
  int argc,
  char *argv[]) {

  ctl_t ctl;

  met_t *met;

  FILE *out;

  static double timem[NZ], z, z0, z1, dz, lon, lon0, lon1, dlon, lonm[NZ],
    lat, lat0, lat1, dlat, latm[NZ], t, tm[NZ], u, um[NZ], v, vm[NZ], w,
    wm[NZ], h2o, h2om[NZ], o3, o3m[NZ], ps, psm[NZ], pt, ptm[NZ], tt, ttm[NZ],
    zm[NZ], zt, ztm[NZ], pv, pvm[NZ], plev[NZ];

  static int i, iz, np[NZ], npt[NZ], nz;

  /* Allocate... */
  ALLOC(met, met_t, 1);

  /* Check arguments... */
  if (argc < 4)
    ERRMSG("Give parameters: <ctl> <prof.tab> <met0> [ <met1> ... ]");

  /* Read control parameters... */
  read_ctl(argv[1], argc, argv, &ctl);
  z0 = scan_ctl(argv[1], argc, argv, "PROF_Z0", -1, "-999", NULL);
  z1 = scan_ctl(argv[1], argc, argv, "PROF_Z1", -1, "-999", NULL);
  dz = scan_ctl(argv[1], argc, argv, "PROF_DZ", -1, "-999", NULL);
  lon0 = scan_ctl(argv[1], argc, argv, "PROF_LON0", -1, "", NULL);
  lon1 = scan_ctl(argv[1], argc, argv, "PROF_LON1", -1, "", NULL);
  dlon = scan_ctl(argv[1], argc, argv, "PROF_DLON", -1, "-999", NULL);
  lat0 = scan_ctl(argv[1], argc, argv, "PROF_LAT0", -1, "", NULL);
  lat1 = scan_ctl(argv[1], argc, argv, "PROF_LAT1", -1, "", NULL);
  dlat = scan_ctl(argv[1], argc, argv, "PROF_DLAT", -1, "-999", NULL);

  /* Loop over input files... */
  for (i = 3; i < argc; i++) {

    /* Read meteorological data... */
    if (!read_met(&ctl, argv[i], met))
      continue;

    /* Set vertical grid... */
    if (z0 < 0)
      z0 = Z(met->p[0]);
    if (z1 < 0)
      z1 = Z(met->p[met->np - 1]);
    nz = 0;
    if (dz < 0) {
      for (iz = 0; iz < met->np; iz++)
	if (Z(met->p[iz]) >= z0 && Z(met->p[iz]) <= z1) {
	  plev[nz] = met->p[iz];
	  if ((++nz) > NZ)
	    ERRMSG("Too many pressure levels!");
	}
    } else
      for (z = z0; z <= z1; z += dz) {
	plev[nz] = P(z);
	if ((++nz) > NZ)
	  ERRMSG("Too many pressure levels!");
      }

    /* Set horizontal grid... */
    if (dlon <= 0)
      dlon = fabs(met->lon[1] - met->lon[0]);
    if (dlat <= 0)
      dlat = fabs(met->lat[1] - met->lat[0]);

    /* Average... */
    for (iz = 0; iz < nz; iz++)
      for (lon = lon0; lon <= lon1; lon += dlon)
	for (lat = lat0; lat <= lat1; lat += dlat) {
	  intpol_met_space(met, plev[iz], lon, lat, &ps, &pt, &z,
			   &t, &u, &v, &w, &pv, &h2o, &o3);
	  intpol_met_space(met, pt, lon, lat, NULL, NULL, &zt,
			   &tt, NULL, NULL, NULL, NULL, NULL, NULL);
	  if (gsl_finite(t) && gsl_finite(u)
	      && gsl_finite(v) && gsl_finite(w)) {
	    timem[iz] += met->time;
	    lonm[iz] += lon;
	    latm[iz] += lat;
	    zm[iz] += z;
	    tm[iz] += t;
	    um[iz] += u;
	    vm[iz] += v;
	    wm[iz] += w;
	    pvm[iz] += pv;
	    h2om[iz] += h2o;
	    o3m[iz] += o3;
	    psm[iz] += ps;
	    if (gsl_finite(pt)) {
	      ptm[iz] += pt;
	      ztm[iz] += zt;
	      ttm[iz] += tt;
	      npt[iz]++;
	    }
	    np[iz]++;
	  }
	}
  }

  /* Create output file... */
  printf("Write meteorological data file: %s\n", argv[2]);
  if (!(out = fopen(argv[2], "w")))
    ERRMSG("Cannot create file!");

  /* Write header... */
  fprintf(out,
	  "# $1 = time [s]\n"
	  "# $2 = altitude [km]\n"
	  "# $3 = longitude [deg]\n"
	  "# $4 = latitude [deg]\n"
	  "# $5 = pressure [hPa]\n"
	  "# $6 = temperature [K]\n"
	  "# $7 = zonal wind [m/s]\n"
	  "# $8 = meridional wind [m/s]\n" "# $9 = vertical wind [hPa/s]\n");
  fprintf(out,
	  "# $10 = H2O volume mixing ratio [1]\n"
	  "# $11 = O3 volume mixing ratio [1]\n"
	  "# $12 = geopotential height [km]\n"
	  "# $13 = potential vorticity [PVU]\n"
	  "# $14 = surface pressure [hPa]\n"
	  "# $15 = tropopause pressure [hPa]\n"
	  "# $16 = tropopause geopotential height [km]\n"
	  "# $17 = tropopause temperature [K]\n\n");

  /* Write data... */
  for (iz = 0; iz < nz; iz++)
    fprintf(out, "%.2f %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g\n",
	    timem[iz] / np[iz], Z(plev[iz]), lonm[iz] / np[iz],
	    latm[iz] / np[iz], plev[iz], tm[iz] / np[iz], um[iz] / np[iz],
	    vm[iz] / np[iz], wm[iz] / np[iz], h2om[iz] / np[iz],
	    o3m[iz] / np[iz], zm[iz] / np[iz], pvm[iz] / np[iz],
	    psm[iz] / np[iz], ptm[iz] / npt[iz], ztm[iz] / npt[iz],
	    ttm[iz] / npt[iz]);

  /* Close file... */
  fclose(out);

  /* Free... */
  free(met);

  return EXIT_SUCCESS;
}
