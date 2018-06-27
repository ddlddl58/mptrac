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
  
  Copright (C) 2013-2015 Forschungszentrum Juelich GmbH
*/

/*! 
  \file
  Calculate transport deviations of trajectories.
*/

#include "libtrac.h"

int main(
  int argc,
  char *argv[]) {

  ctl_t ctl;

  atm_t *atm1, *atm2;

  FILE *out;

  char tstr[LEN];

  double aux, x0[3], x1[3], x2[3], *lon1, *lat1, *p1, *lh1, *lv1,
    *lon2, *lat2, *p2, *lh2, *lv2, ahtd, avtd, ahtd2, avtd2,
    aqtd[NQ], aqtd2[NQ], rhtd, rvtd, rhtd2, rvtd2, t, *dh, *dv, dq;

  int f, ip, iph, ipv, iq, year, mon, day, hour, min;

  /* Allocate... */
  ALLOC(atm1, atm_t, 1);
  ALLOC(atm2, atm_t, 1);
  ALLOC(lon1, double,
	NP);
  ALLOC(lat1, double,
	NP);
  ALLOC(p1, double,
	NP);
  ALLOC(lh1, double,
	NP);
  ALLOC(lv1, double,
	NP);
  ALLOC(lon2, double,
	NP);
  ALLOC(lat2, double,
	NP);
  ALLOC(p2, double,
	NP);
  ALLOC(lh2, double,
	NP);
  ALLOC(lv2, double,
	NP);
  ALLOC(dh, double,
	NP);
  ALLOC(dv, double,
	NP);

  /* Check arguments... */
  if (argc < 5)
    ERRMSG("Give parameters: <ctl> <outfile> <atm1a> <atm1b>"
	   " [<atm2a> <atm2b> ...]");

  /* Read control parameters... */
  read_ctl(argv[1], argc, argv, &ctl);

  /* Write info... */
  printf("Write transport deviations: %s\n", argv[2]);

  /* Create output file... */
  if (!(out = fopen(argv[2], "w")))
    ERRMSG("Cannot create file!");

  /* Write header... */
  fprintf(out,
	  "# $1  = time [s]\n"
	  "# $2  = AHTD (mean) [km]\n"
	  "# $3  = AHTD (sigma) [km]\n"
	  "# $4  = AHTD (minimum) [km]\n"
	  "# $5  = AHTD (10%% percentile) [km]\n"
	  "# $6  = AHTD (1st quartile) [km]\n"
	  "# $7  = AHTD (median) [km]\n"
	  "# $8  = AHTD (3rd quartile) [km]\n"
	  "# $9  = AHTD (90%% percentile) [km]\n"
	  "# $10 = AHTD (maximum) [km]\n"
	  "# $11 = AHTD (maximum trajectory index)\n"
	  "# $12 = RHTD (mean) [%%]\n" "# $13 = RHTD (sigma) [%%]\n");
  fprintf(out,
	  "# $14 = AVTD (mean) [km]\n"
	  "# $15 = AVTD (sigma) [km]\n"
	  "# $16 = AVTD (minimum) [km]\n"
	  "# $17 = AVTD (10%% percentile) [km]\n"
	  "# $18 = AVTD (1st quartile) [km]\n"
	  "# $19 = AVTD (median) [km]\n"
	  "# $20 = AVTD (3rd quartile) [km]\n"
	  "# $21 = AVTD (90%% percentile) [km]\n"
	  "# $22 = AVTD (maximum) [km]\n"
	  "# $23 = AVTD (maximum trajectory index)\n"
	  "# $24 = RVTD (mean) [%%]\n" "# $25 = RVTD (sigma) [%%]\n");
  for (iq = 0; iq < ctl.nq; iq++)
    fprintf(out,
	    "# $%d = %s transport deviation (mean) [%s]\n"
	    "# $%d = %s transport deviation (sigma) [%s]\n",
	    26 + 2 * iq, ctl.qnt_name[iq], ctl.qnt_unit[iq],
	    27 + 2 * iq, ctl.qnt_name[iq], ctl.qnt_unit[iq]);
  fprintf(out, "\n");

  /* Loop over file pairs... */
  for (f = 3; f < argc; f += 2) {

    /* Read atmopheric data... */
    read_atm(argv[f], &ctl, atm1);
    read_atm(argv[f + 1], &ctl, atm2);

    /* Check if structs match... */
    if (atm1->np != atm2->np)
      ERRMSG("Different numbers of parcels!");
    for (ip = 0; ip < atm1->np; ip++)
      if (atm1->time[ip] != atm2->time[ip])
	ERRMSG("Times do not match!");

    /* Init... */
    ahtd = ahtd2 = 0;
    avtd = avtd2 = 0;
    rhtd = rhtd2 = 0;
    rvtd = rvtd2 = 0;
    for (iq = 0; iq < ctl.nq; iq++)
      aqtd[iq] = aqtd2[iq] = 0;

    /* Loop over air parcels... */
    for (ip = 0; ip < atm1->np; ip++) {

      /* Get Cartesian coordinates... */
      geo2cart(0, atm1->lon[ip], atm1->lat[ip], x1);
      geo2cart(0, atm2->lon[ip], atm2->lat[ip], x2);

      /* Calculate absolute transport deviations... */
      dh[ip] = DIST(x1, x2);
      ahtd += dh[ip];
      ahtd2 += gsl_pow_2(dh[ip]);

      dv[ip] = fabs(Z(atm1->p[ip]) - Z(atm2->p[ip]));
      avtd += dv[ip];
      avtd2 += gsl_pow_2(dv[ip]);

      for (iq = 0; iq < ctl.nq; iq++) {
	dq = fabs(atm1->q[iq][ip] - atm2->q[iq][ip]);
	aqtd[iq] += dq;
	aqtd2[iq] += gsl_pow_2(dq);
      }

      /* Calculate relative transport deviations... */
      if (f > 2) {

	/* Get trajectory lengths... */
	geo2cart(0, lon1[ip], lat1[ip], x0);
	lh1[ip] += DIST(x0, x1);
	lv1[ip] += fabs(Z(p1[ip]) - Z(atm1->p[ip]));

	geo2cart(0, lon2[ip], lat2[ip], x0);
	lh2[ip] += DIST(x0, x2);
	lv2[ip] += fabs(Z(p2[ip]) - Z(atm2->p[ip]));

	/* Get relative transport devations... */
	if (lh1[ip] + lh2[ip] > 0) {
	  aux = 200. * DIST(x1, x2) / (lh1[ip] + lh2[ip]);
	  rhtd += aux;
	  rhtd2 += gsl_pow_2(aux);
	}
	if (lv1[ip] + lv2[ip] > 0) {
	  aux =
	    200. * fabs(Z(atm1->p[ip]) - Z(atm2->p[ip])) / (lv1[ip] +
							    lv2[ip]);
	  rvtd += aux;
	  rvtd2 += gsl_pow_2(aux);
	}
      }

      /* Save positions of air parcels... */
      lon1[ip] = atm1->lon[ip];
      lat1[ip] = atm1->lat[ip];
      p1[ip] = atm1->p[ip];

      lon2[ip] = atm2->lon[ip];
      lat2[ip] = atm2->lat[ip];
      p2[ip] = atm2->p[ip];
    }

    /* Get indices of trajectories with maximum errors... */
    iph = (int) gsl_stats_max_index(dh, 1, (size_t) atm1->np);
    ipv = (int) gsl_stats_max_index(dv, 1, (size_t) atm1->np);

    /* Sort distances to calculate percentiles... */
    gsl_sort(dh, 1, (size_t) atm1->np);
    gsl_sort(dv, 1, (size_t) atm1->np);

    /* Get time from filename... */
    sprintf(tstr, "%.4s", &argv[f][strlen(argv[f]) - 20]);
    year = atoi(tstr);
    sprintf(tstr, "%.2s", &argv[f][strlen(argv[f]) - 15]);
    mon = atoi(tstr);
    sprintf(tstr, "%.2s", &argv[f][strlen(argv[f]) - 12]);
    day = atoi(tstr);
    sprintf(tstr, "%.2s", &argv[f][strlen(argv[f]) - 9]);
    hour = atoi(tstr);
    sprintf(tstr, "%.2s", &argv[f][strlen(argv[f]) - 6]);
    min = atoi(tstr);
    time2jsec(year, mon, day, hour, min, 0, 0, &t);

    /* Write output... */
    fprintf(out, "%.2f %g %g %g %g %g %g %g %g %g %d %g %g"
	    " %g %g %g %g %g %g %g %g %g %d %g %g", t,
	    ahtd / atm1->np,
	    sqrt(ahtd2 / atm1->np - gsl_pow_2(ahtd / atm1->np)),
	    dh[0], dh[atm1->np / 10], dh[atm1->np / 4], dh[atm1->np / 2],
	    dh[atm1->np - atm1->np / 4], dh[atm1->np - atm1->np / 10],
	    dh[atm1->np - 1], iph, rhtd / atm1->np,
	    sqrt(rhtd2 / atm1->np - gsl_pow_2(rhtd / atm1->np)),
	    avtd / atm1->np,
	    sqrt(avtd2 / atm1->np - gsl_pow_2(avtd / atm1->np)),
	    dv[0], dv[atm1->np / 10], dv[atm1->np / 4], dv[atm1->np / 2],
	    dv[atm1->np - atm1->np / 4], dv[atm1->np - atm1->np / 10],
	    dv[atm1->np - 1], ipv, rvtd / atm1->np,
	    sqrt(rvtd2 / atm1->np - gsl_pow_2(rvtd / atm1->np)));
    for (iq = 0; iq < ctl.nq; iq++) {
      fprintf(out, " ");
      fprintf(out, ctl.qnt_format[iq], aqtd[iq] / atm1->np);
      fprintf(out, " ");
      fprintf(out, ctl.qnt_format[iq],
	      sqrt(aqtd2[iq] / atm1->np - gsl_pow_2(aqtd[iq] / atm1->np)));
    }
    fprintf(out, "\n");
  }

  /* Close file... */
  fclose(out);

  /* Free... */
  free(atm1);
  free(atm2);
  free(lon1);
  free(lat1);
  free(p1);
  free(lh1);
  free(lv1);
  free(lon2);
  free(lat2);
  free(p2);
  free(lh2);
  free(lv2);
  free(dh);
  free(dv);

  return EXIT_SUCCESS;
}
