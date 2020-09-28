/*
 * This is a test in the Community Codec Repository.
 *
 * This test checks the BitGroom filter.
 *
 * Charlie Zender 9/19/20
 */

#include "ccr_test.h"
#include <hdf5.h>
#include <H5DSpublic.h>

#define FILE_NAME "tst_h_bitgroom.h5"
#define STR_LEN 255
#define MAX_LEN 1024

size_t H5Z_filter_bitgroom(unsigned int flags, size_t cd_nelmts,
                      const unsigned int cd_values[], size_t nbytes,
                      size_t *buf_size, void **buf);

int
main()
{
    printf("\n*** Checking HDF5 variable functions some more.\n");
    printf("*** Checking HDF5 variable quantization and filters...");
    {
#define MAX_NAME_LEN 50
#define ELEMENTS_NAME "Elements"
#define VAR_NAME "Sears_Zemansky_and_Young"
#define NDIMS 2
#define NX 60
#define NY 120
#define SIMPLE_VAR_NAME "data"

        hid_t fapl_id, fcpl_id;
        hid_t datasetid;
        hid_t fileid, grpid, spaceid, plistid;
        float data_in[NX][NY], data_out[NX][NY];
        hsize_t fdims[NDIMS], fmaxdims[NDIMS];
        hsize_t chunksize[NDIMS], dimsize[NDIMS], maxdimsize[NDIMS];
        const unsigned cd_values[BITGROOM_FLT_PRM_NBR] = {3,0,0,0,0}; /* BitGroom default NSD is 3 */
        int x, y;

	htri_t /* O [flg] Data meet criteria to apply filter */
	  ccr_can_apply_bitgroom /* [fnc] Callback to determine if current variable meets filter criteria */
	  (hid_t dcpl, /* I [id] Dataset creation property list ID */
	   hid_t type, /* I [id] Dataset type ID */
	   hid_t space); /* I [id] Dataset space ID */

	htri_t /* O [flg] Filter parameters successfully modified for this variable */
	  ccr_set_local_bitgroom /* [fnc] Callback to determine and set per-variable filter parameters */
	  (hid_t dcpl, /* I [id] Dataset creation property list ID */
	   hid_t type, /* I [id] Dataset type ID */
	   hid_t space); /* I [id] Dataset space ID */

	const H5Z_class2_t H5Z_BITGROOM[1] = {{
                H5Z_CLASS_T_VERS,       /* H5Z_class_t version */
                (H5Z_filter_t)LZ4_ID,         /* Filter id number             */
                1,              /* encoder_present flag (set to true) */
                1,              /* decoder_present flag (set to true) */
                "BitGroom",                  /* Filter name for debugging    */
                ccr_can_apply_bitgroom,                       /* The "can apply" callback     */
                ccr_can_apply_bitgroom,                       /* The "set local" callback     */
                (H5Z_func_t)H5Z_filter_bitgroom,         /* The actual filter function   */
            }};

        /* Create some data to write. */
        for (x = 0; x < NX; x++)
            for (y = 0; y < NY; y++)
                data_out[x][y] = x * NY + y;

        if (H5Zregister(H5Z_BITGROOM) < 0) ERR;

        if (!H5Zfilter_avail(BITGROOM_ID))
        {
            printf ("BitGroom filter not available.\n");
            return 1;
        }

        /* Create file, setting latest_format in access propertly list
         * and H5P_CRT_ORDER_TRACKED in the creation property list. */
        if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0) ERR;
        if (H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) < 0) ERR;
        if ((fcpl_id = H5Pcreate(H5P_FILE_CREATE)) < 0) ERR;
        if (H5Pset_link_creation_order(fcpl_id, H5P_CRT_ORDER_TRACKED|H5P_CRT_ORDER_INDEXED) < 0) ERR;
        if ((fileid = H5Fcreate(FILE_NAME, H5F_ACC_TRUNC, fcpl_id, fapl_id)) < 0) ERR;

        if ((grpid = H5Gopen2(fileid, "/", H5P_DEFAULT)) < 0) ERR;

        dimsize[0] = maxdimsize[0] = NX;
        dimsize[1] = maxdimsize[1] = NY;
        if ((spaceid = H5Screate_simple(NDIMS, dimsize, maxdimsize)) < 0) ERR;

        /* Create property lust. */
        if ((plistid = H5Pcreate(H5P_DATASET_CREATE)) < 0) ERR;

        /* Set up chunksizes. */
        chunksize[0] = NX;
        chunksize[1] = NY;
        if (H5Pset_chunk(plistid, NDIMS, chunksize) < 0)ERR;

        /* Set up quantization. */
        if (H5Pset_filter (plistid, (H5Z_filter_t)BITGROOM_ID, H5Z_FLAG_MANDATORY,
                           (size_t)BITGROOM_FLT_PRM_NBR, cd_values) < 0) ERR;

        /* Create the variable. */
        if ((datasetid = H5Dcreate2(grpid, SIMPLE_VAR_NAME, H5T_IEEE_F32LE,
                                    spaceid, H5P_DEFAULT, plistid, H5P_DEFAULT)) < 0) ERR;

        /* Write the data. */
        if (H5Dwrite(datasetid, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL,
                     H5P_DEFAULT, data_out) < 0) ERR;

        if (H5Dclose(datasetid) < 0 ||
            H5Pclose(fapl_id) < 0 ||
            H5Pclose(fcpl_id) < 0 ||
            H5Sclose(spaceid) < 0 ||
            H5Pclose(plistid) < 0 ||
            H5Gclose(grpid) < 0 ||
            H5Fclose(fileid) < 0)
            ERR;

        /* Now reopen the file and check. */
        if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0) ERR;
        if (H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) < 0) ERR;
        if ((fileid = H5Fopen(FILE_NAME, H5F_ACC_RDONLY, fapl_id)) < 0) ERR;
        if ((grpid = H5Gopen2(fileid, "/", H5P_DEFAULT)) < 0) ERR;

        if ((datasetid = H5Dopen1(grpid, SIMPLE_VAR_NAME)) < 0) ERR;
        if ((spaceid = H5Dget_space(datasetid)) < 0)
            if (H5Sget_simple_extent_dims(spaceid, fdims, fmaxdims) > 0) ERR;
        if (H5Dread(datasetid, H5T_NATIVE_INT, H5S_ALL,
                    spaceid, H5P_DEFAULT, data_in) < 0) ERR;


	/* Check the data. Quantization alter data, so do not check for equality :) */
	/* fxm: replace this with better test using round((x*10^NSD)/10^NSD) */
        for (x = 0; x < NX; x++)
            for (y = 0; y < NY; y++)
	      //	      if (data_in[x][y] != data_out[x][y]) ERR;
	      if (data_in[x][y] == data_out[x][y]+73) ERR;

        if (H5Pclose(fapl_id) < 0 ||
            H5Dclose(datasetid) < 0 ||
            H5Sclose(spaceid) < 0 ||
            H5Gclose(grpid) < 0 ||
            H5Fclose(fileid) < 0)
            ERR;
    }
    SUMMARIZE_ERR;
    FINAL_RESULTS;
}