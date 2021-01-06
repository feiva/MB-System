/*--------------------------------------------------------------------
 *    The MB-system:    mbimagecorrect.cpp    10/17/2013
 *
 *    Copyright (c) 2020-2020 by
 *    David W. Caress (caress@mbari.org)
 *      Monterey Bay Aquarium Research Institute
 *      Moss Landing, CA 95039
 *    and Dale N. Chayes (dale@ldeo.columbia.edu)
 *      Lamont-Doherty Earth Observatory
 *      Palisades, NY 10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------
 *
 * mbimagecorrect applies some simple brightness and contrast
 * corrections to images using standard OpenCV algorithms.
 *
 * Author:    D. W. Caress
 * Date:    November 6, 2020
 *
 *--------------------------------------------------------------------
 *
 * OpenCV code structure and CLAHE algorithm obtained from:
 *   https://stackoverflow.com/questions/24341114/simple-illumination-correction-in-images-opencv-c/24341809#24341809
 *
 * Gamma correction obtained from:
 *   https://github.com/DynamsoftRD/opencv-programming/blob/master/gamma-correction/gamma.cpp
 *
 * Note that OpenCV has arbitrary maximum image dimensions that are enforced when
 * reading images (but not when defining or writing images):
 *   size_t CV_IO_MAX_IMAGE_WIDTH   1 << 20 == 2^20 == 1048576
 *   size_t CV_IO_MAX_IMAGE_HEIGHT  1 << 20 == 2^20 == 1048576
 *   size_t CV_IO_MAX_IMAGE_PIXELS  1 << 30 == 2^30 == 1073741824
 * To work with images exceeding these limits set shell environment variables before
 * running the program, for instance to allow images with 2^40 (~1 trillion) pixels:
 *   export OPENCV_IO_MAX_IMAGE_PIXELS=1099511627776
 *
 */

#include <getopt.h>
#include <vector>

/* MB-System include files */
extern "C"
{
#include "mb_define.h"
#include "mb_status.h"
}

/* OpenCV include files */
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

#define MB_IMAGE_CORR_MODE_NONE     0
#define MB_IMAGE_CORR_MODE_HISTEQ   1
#define MB_IMAGE_CORR_MODE_GAMMA    2
#define MB_IMAGE_CORR_MODE_CLAHE    3

int main(int argc, char** argv)
{
    char program_name[] = "mbimagecorrect";
    char help_message[] =  "mbimagecorrect applies simple image corrections to an image";
    char usage_message[] = "mbimagecorrect \n"
                            "\t--verbose\n"
                            "\t--help\n"
                            "\t--input=imagelist || --input=imagefile\n"
                            "\t--output=imagefile\n"
                            "\t--histogram-equalization\n"
                            "\t--gamma-correction\n"
                            "\t--clahe-correction\n";

    /* MBIO status variables */
    int status = MB_SUCCESS;
    int verbose = 0;
    int error = MB_ERROR_NO_ERROR;
    bool help = false;
    char *message;

    /* output stream for basic stuff (stdout if verbose <= 1,
        stderr if verbose > 1) */
    FILE *stream = NULL;
    FILE *tfp;

    static struct option options[] = {
      {"verbose",                     no_argument,            NULL, 0},
      {"help",                        no_argument,            NULL, 0},
      {"input",                       required_argument,      NULL, 0},
      {"output",                      required_argument,      NULL, 0},
      {"histogram-equalization",      no_argument,            NULL, 0},
      {"gamma-correction",            required_argument,      NULL, 0},
      {"clahe-correction",            no_argument,            NULL, 0}
    };

    /* declare input and output */
    mb_path InputImageFile, OutputImageFile;
    memset(InputImageFile, 0, sizeof(mb_path));
    memset(OutputImageFile, 0, sizeof(mb_path));

    /* correction parameters */
    int correction_mode = MB_IMAGE_CORR_MODE_NONE;
    double gamma = 0.5;

    /* process argument list */
    extern char *optarg;
    int option_index = 0;
    int errflg = 0;
    int c;
    while ((c = getopt_long(argc, argv, "", options, &option_index)) != -1)
      switch (c) {
        /* long options all return c=0 */
        case 0:
            /* verbose */
            if (strcmp("verbose", options[option_index].name) == 0) {
                verbose++;
            }

            /* help */
            else if (strcmp("help", options[option_index].name) == 0) {
                help = true;
            }

            /* input */
            else if (strcmp("input", options[option_index].name) == 0) {
                sscanf (optarg,"%s", InputImageFile);
            }

            /* output */
            else if (strcmp("output", options[option_index].name) == 0) {
                sscanf (optarg,"%s", OutputImageFile);
            }

            /* histogram-equalization */
            else if (strcmp("histogram-equalization", options[option_index].name) == 0) {
                correction_mode = MB_IMAGE_CORR_MODE_HISTEQ;
            }

            /* gamma-correction */
            else if (strcmp("gamma-correction", options[option_index].name) == 0) {
                sscanf (optarg,"%lf", &gamma);
                correction_mode = MB_IMAGE_CORR_MODE_GAMMA;
            }

            /* clahe-correction */
            else if (strcmp("clahe-correction", options[option_index].name) == 0) {
                correction_mode = MB_IMAGE_CORR_MODE_CLAHE;
            }

            break;
        case '?':
            errflg++;
      }

    /* set default correction mode */
    if (correction_mode == MB_IMAGE_CORR_MODE_NONE)
      correction_mode = MB_IMAGE_CORR_MODE_CLAHE;

    /* if error flagged then print it and exit */
    if (errflg) {
      fprintf(stderr,"usage: %s\n", usage_message);
      fprintf(stderr,"\nProgram <%s> Terminated\n",
          program_name);
      error = MB_ERROR_BAD_USAGE;
      exit(error);
    }

    /* set output stream */
    if (verbose <= 1)
      stream = stdout;
    else
      stream = stderr;

    /* print starting message */
    if (verbose == 1 || help) {
      fprintf(stream,"\nProgram %s\n",program_name);
      fprintf(stream,"MB-system Version %s\n",MB_VERSION);
    }


    /* print starting debug statements */
    if (verbose >= 2) {
      fprintf(stream,"\ndbg2  Program <%s>\n",program_name);
      fprintf(stream,"dbg2  MB-system Version %s\n",MB_VERSION);
      fprintf(stream,"dbg2  Control Parameters:\n");
      fprintf(stream,"dbg2       verbose:                     %d\n",verbose);
      fprintf(stream,"dbg2       help:                        %d\n",help);
      fprintf(stream,"dbg2       InputImageFile:              %s\n",InputImageFile);
      fprintf(stream,"dbg2       OutputImageFile:             %s\n",OutputImageFile);
      fprintf(stream,"dbg2       correction_mode:             %d\n",correction_mode);
    }

    // Read input RGB image
    Mat src_img = imread(InputImageFile);
    Mat dst_img;

    switch (correction_mode) {
      case MB_IMAGE_CORR_MODE_HISTEQ:
        {
        //Convert the image from BGR to YCrCb color space
        cvtColor(src_img, dst_img, COLOR_BGR2YCrCb);

        //Split the image into 3 channels; Y, Cr and Cb channels respectively and store it in a std::vector
        vector<Mat> vec_channels;
        split(dst_img, vec_channels);

        //Equalize the histogram of only the Y channel
        equalizeHist(vec_channels[0], vec_channels[0]);

        //Merge 3 channels in the vector to form the color image in YCrCB color space.
        merge(vec_channels, dst_img);

        //Convert the histogram equalized image from YCrCb to BGR color space again
        cvtColor(dst_img, dst_img, COLOR_YCrCb2BGR);

        break;
        }

      case MB_IMAGE_CORR_MODE_GAMMA:
        {
        // build look up table
        unsigned char lut[256];
        for (int i = 0; i < 256; i++) {
          lut[i] = saturate_cast<uchar>(pow((float)(i / 255.0), gamma) * 255.0f);
        }

        // apply gamma correction
        dst_img = src_img.clone();
        MatIterator_<Vec3b> it, end;
        for (it = dst_img.begin<Vec3b>(), end = dst_img.end<Vec3b>(); it != end; it++) {
          (*it)[0] = lut[((*it)[0])];
          (*it)[1] = lut[((*it)[1])];
          (*it)[2] = lut[((*it)[2])];
        }

        break;
        }

      case MB_IMAGE_CORR_MODE_CLAHE:
        {
        Mat lab_img;

        // convert RGB color image to Lab
        cvtColor(src_img, lab_img, COLOR_BGR2Lab);

        // Extract the L channel
        vector<cv::Mat> lab_planes(3);
        split(lab_img, lab_planes);  // now we have the L image in lab_planes[0]

        // apply the CLAHE algorithm to the L channel
        Ptr<cv::CLAHE> clahe = createCLAHE();
        clahe->setClipLimit(4);
        Mat tmp;
        clahe->apply(lab_planes[0], tmp);

        // Merge the the color planes back into an Lab image
        tmp.copyTo(lab_planes[0]);
        merge(lab_planes, lab_img);

        // convert back to RGB
        cvtColor(lab_img, dst_img, COLOR_Lab2BGR);

        break;
        }
    }

   imwrite(OutputImageFile, dst_img);

   // display the results  (you might also want to see lab_planes[0] before and after).
   imshow("Original Image", src_img);
   imshow("Corrected Image", dst_img);
   waitKey();

    /* end it all */
    exit(0);

}