/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.

                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
Third party copyrights are property of their respective owners.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#include "precomp.hpp"
#include "opencv2/aruco/dictionary.hpp"
#include "predefined_dictionaries.cpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>


namespace cv {
namespace aruco {

using namespace std;



/**
  * Hamming weight look up table from 0 to 255
  */
const unsigned char hammingWeightLUT[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};



/**
  */
Dictionary::Dictionary(const unsigned char *bytes, int _markerSize, int dictsize, int _maxcorr) {
    markerSize = _markerSize;
    maxCorrectionBits = _maxcorr;
    int nbytes = (markerSize * markerSize) / 8;
    if((markerSize * markerSize) % 8 != 0) nbytes++;

    // save bytes in internal format
    // bytesList.at<Vec4b>(i, j)[k] is j-th byte of i-th marker, in its k-th rotation
    bytesList = Mat(dictsize, nbytes, CV_8UC4);
    for(int i = 0; i < dictsize; i++) {
        for(int j = 0; j < nbytes; j++) {
            for(int k = 0; k < 4; k++)
                bytesList.at< Vec4b >(i, j)[k] = bytes[i * (4 * nbytes) + k * nbytes + j];
        }
    }
}



/**
 */
bool Dictionary::identify(const Mat &onlyBits, int &idx, int &rotation,
                          double maxCorrectionRate) const {

    CV_Assert(onlyBits.rows == markerSize && onlyBits.cols == markerSize);

    int maxCorrectionRecalculed = int(double(maxCorrectionBits) * maxCorrectionRate);

    // get as a byte list
    Mat candidateBytes = getByteListFromBits(onlyBits);

    idx = -1; // by default, not found

    // search closest marker in dict
    for(int m = 0; m < bytesList.rows; m++) {
        int currentMinDistance = markerSize * markerSize + 1;
        int currentRotation = -1;
        for(unsigned int r = 0; r < 4; r++) {
            int currentHamming = 0;
            // for each byte, calculate XOR result and then sum the Hamming weight from the LUT
            for(unsigned int b = 0; b < candidateBytes.total(); b++) {
                unsigned char xorRes =
                    bytesList.ptr< Vec4b >(m)[b][r] ^ candidateBytes.ptr< Vec4b >(0)[b][0];
                currentHamming += hammingWeightLUT[xorRes];
            }

            if(currentHamming < currentMinDistance) {
                currentMinDistance = currentHamming;
                currentRotation = r;
            }
        }

        // if maxCorrection is fullfilled, return this one
        if(currentMinDistance <= maxCorrectionRecalculed) {
            idx = m;
            rotation = currentRotation;
            break;
        }
    }

    if(idx != -1)
        return true;
    else
        return false;
}


/**
  */
int Dictionary::getDistanceToId(InputArray bits, int id, bool allRotations) const {

    CV_Assert(id >= 0 && id < bytesList.rows);

    unsigned int nRotations = 4;
    if(!allRotations) nRotations = 1;

    Mat candidateBytes = getByteListFromBits(bits.getMat());
    int currentMinDistance = int(bits.total() * bits.total());
    for(unsigned int r = 0; r < nRotations; r++) {
        int currentHamming = 0;
        for(unsigned int b = 0; b < candidateBytes.total(); b++) {
            unsigned char xorRes =
                bytesList.ptr< Vec4b >(id)[b][r] ^ candidateBytes.ptr< Vec4b >(0)[b][0];
            currentHamming += hammingWeightLUT[xorRes];
        }

        if(currentHamming < currentMinDistance) {
            currentMinDistance = currentHamming;
        }
    }
    return currentMinDistance;
}



/**
 * @brief Draw a canonical marker image
 */
void Dictionary::drawMarker(int id, int sidePixels, OutputArray _img, int borderBits) const {

    CV_Assert(sidePixels > markerSize);
    CV_Assert(id < bytesList.rows);
    CV_Assert(borderBits > 0);

    _img.create(sidePixels, sidePixels, CV_8UC1);

    // create small marker with 1 pixel per bin
    Mat tinyMarker(markerSize + 2 * borderBits, markerSize + 2 * borderBits, CV_8UC1,
                   Scalar::all(0));
    Mat innerRegion = tinyMarker.rowRange(borderBits, tinyMarker.rows - borderBits)
                          .colRange(borderBits, tinyMarker.cols - borderBits);
    // put inner bits
    Mat bits = 255 * getBitsFromByteList(bytesList.rowRange(id, id + 1), markerSize);
    CV_Assert(innerRegion.total() == bits.total());
    bits.copyTo(innerRegion);

    // resize tiny marker to output size
    cv::resize(tinyMarker, _img.getMat(), _img.getMat().size(), 0, 0, INTER_NEAREST);
}




/**
  * @brief Transform matrix of bits to list of bytes in the 4 rotations
  */
Mat Dictionary::getByteListFromBits(const Mat &bits) {

    int nbytes = (bits.cols * bits.rows) / 8;
    if((bits.cols * bits.rows) % 8 != 0) nbytes++;

    Mat candidateByteList(1, nbytes, CV_8UC4, Scalar::all(0));
    unsigned char currentBit = 0;
    int currentByte = 0;
    for(int row = 0; row < bits.rows; row++) {
        for(int col = 0; col < bits.cols; col++) {
            // circular shift
            candidateByteList.ptr< Vec4b >(0)[currentByte][0] =
                candidateByteList.ptr< Vec4b >(0)[currentByte][0] << 1;
            candidateByteList.ptr< Vec4b >(0)[currentByte][1] =
                candidateByteList.ptr< Vec4b >(0)[currentByte][1] << 1;
            candidateByteList.ptr< Vec4b >(0)[currentByte][2] =
                candidateByteList.ptr< Vec4b >(0)[currentByte][2] << 1;
            candidateByteList.ptr< Vec4b >(0)[currentByte][3] =
                candidateByteList.ptr< Vec4b >(0)[currentByte][3] << 1;
            // increment if bit is 1
            if(bits.at< unsigned char >(row, col))
                candidateByteList.ptr< Vec4b >(0)[currentByte][0]++;
            if(bits.at< unsigned char >(col, bits.cols - 1 - row))
                candidateByteList.ptr< Vec4b >(0)[currentByte][1]++;
            if(bits.at< unsigned char >(bits.rows - 1 - row, bits.cols - 1 - col))
                candidateByteList.ptr< Vec4b >(0)[currentByte][2]++;
            if(bits.at< unsigned char >(bits.rows - 1 - col, row))
                candidateByteList.ptr< Vec4b >(0)[currentByte][3]++;
            currentBit++;
            if(currentBit == 8) {
                // next byte
                currentBit = 0;
                currentByte++;
            }
        }
    }
    return candidateByteList;
}



/**
  * @brief Transform list of bytes to matrix of bits
  */
Mat Dictionary::getBitsFromByteList(const Mat &byteList, int markerSize) {
    CV_Assert(byteList.total() > 0 &&
              byteList.total() >= (unsigned int)markerSize * markerSize / 8 &&
              byteList.total() <= (unsigned int)markerSize * markerSize / 8 + 1);
    Mat bits(markerSize, markerSize, CV_8UC1, Scalar::all(0));

    unsigned char base2List[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    int currentByteIdx = 0;
    // we only need the bytes in normal rotation
    unsigned char currentByte = byteList.ptr< Vec4b >(0)[0][0];
    int currentBit = 0;
    for(int row = 0; row < bits.rows; row++) {
        for(int col = 0; col < bits.cols; col++) {
            if(currentByte >= base2List[currentBit]) {
                bits.at< unsigned char >(row, col) = 1;
                currentByte -= base2List[currentBit];
            }
            currentBit++;
            if(currentBit == 8) {
                currentByteIdx++;
                currentByte = byteList.ptr< Vec4b >(0)[currentByteIdx][0];
                // if not enough bits for one more byte, we are in the end
                // update bit position accordingly
                if(8 * (currentByteIdx + 1) > (int)bits.total())
                    currentBit = 8 * (currentByteIdx + 1) - (int)bits.total();
                else
                    currentBit = 0; // ok, bits enough for next byte
            }
        }
    }
    return bits;
}




// DictionaryData constructors calls
const Dictionary DICT_ARUCO_DATA = Dictionary(&(DICT_ARUCO_BYTES[0][0][0]), 5, 1024, 1);

const Dictionary DICT_4X4_50_DATA = Dictionary(&(DICT_4X4_1000_BYTES[0][0][0]), 4, 50, 1);
const Dictionary DICT_4X4_100_DATA = Dictionary(&(DICT_4X4_1000_BYTES[0][0][0]), 4, 100, 1);
const Dictionary DICT_4X4_250_DATA = Dictionary(&(DICT_4X4_1000_BYTES[0][0][0]), 4, 250, 1);
const Dictionary DICT_4X4_1000_DATA = Dictionary(&(DICT_4X4_1000_BYTES[0][0][0]), 4, 1000, 0);

const Dictionary DICT_5X5_50_DATA = Dictionary(&(DICT_5X5_1000_BYTES[0][0][0]), 5, 50, 3);
const Dictionary DICT_5X5_100_DATA = Dictionary(&(DICT_5X5_1000_BYTES[0][0][0]), 5, 100, 3);
const Dictionary DICT_5X5_250_DATA = Dictionary(&(DICT_5X5_1000_BYTES[0][0][0]), 5, 250, 2);
const Dictionary DICT_5X5_1000_DATA = Dictionary(&(DICT_5X5_1000_BYTES[0][0][0]), 5, 1000, 2);

const Dictionary DICT_6X6_50_DATA = Dictionary(&(DICT_6X6_1000_BYTES[0][0][0]), 6, 50, 6);
const Dictionary DICT_6X6_100_DATA = Dictionary(&(DICT_6X6_1000_BYTES[0][0][0]), 6, 100, 5);
const Dictionary DICT_6X6_250_DATA = Dictionary(&(DICT_6X6_1000_BYTES[0][0][0]), 6, 250, 5);
const Dictionary DICT_6X6_1000_DATA = Dictionary(&(DICT_6X6_1000_BYTES[0][0][0]), 6, 1000, 4);

const Dictionary DICT_7X7_50_DATA = Dictionary(&(DICT_7X7_1000_BYTES[0][0][0]), 7, 50, 9);
const Dictionary DICT_7X7_100_DATA = Dictionary(&(DICT_7X7_1000_BYTES[0][0][0]), 7, 100, 8);
const Dictionary DICT_7X7_250_DATA = Dictionary(&(DICT_7X7_1000_BYTES[0][0][0]), 7, 250, 8);
const Dictionary DICT_7X7_1000_DATA = Dictionary(&(DICT_7X7_1000_BYTES[0][0][0]), 7, 1000, 6);


const Dictionary &getPredefinedDictionary(PREDEFINED_DICTIONARY_NAME name) {
    switch(name) {

    case DICT_ARUCO_ORIGINAL:
        return DICT_ARUCO_DATA;

    case DICT_4X4_50:
        return DICT_4X4_50_DATA;
    case DICT_4X4_100:
        return DICT_4X4_100_DATA;
    case DICT_4X4_250:
        return DICT_4X4_250_DATA;
    case DICT_4X4_1000:
        return DICT_4X4_1000_DATA;

    case DICT_5X5_50:
        return DICT_5X5_50_DATA;
    case DICT_5X5_100:
        return DICT_5X5_100_DATA;
    case DICT_5X5_250:
        return DICT_5X5_250_DATA;
    case DICT_5X5_1000:
        return DICT_5X5_1000_DATA;

    case DICT_6X6_50:
        return DICT_6X6_50_DATA;
    case DICT_6X6_100:
        return DICT_6X6_100_DATA;
    case DICT_6X6_250:
        return DICT_6X6_250_DATA;
    case DICT_6X6_1000:
        return DICT_6X6_1000_DATA;

    case DICT_7X7_50:
        return DICT_7X7_50_DATA;
    case DICT_7X7_100:
        return DICT_7X7_100_DATA;
    case DICT_7X7_250:
        return DICT_7X7_250_DATA;
    case DICT_7X7_1000:
        return DICT_7X7_1000_DATA;

    }
    return DICT_4X4_50_DATA;
}


/**
 * @brief Generates a random marker Mat of size markerSize x markerSize
 */
static Mat _generateRandomMarker(int markerSize) {
    Mat marker(markerSize, markerSize, CV_8UC1, Scalar::all(0));
    for(int i = 0; i < markerSize; i++) {
        for(int j = 0; j < markerSize; j++) {
            unsigned char bit = rand() % 2;
            marker.at< unsigned char >(i, j) = bit;
        }
    }
    return marker;
}

/**
 * @brief Calculate selfDistance of the codification of a marker Mat. Self distance is the Hamming
 * distance of the marker to itself in the other rotations.
 * See S. Garrido-Jurado, R. Muñoz-Salinas, F. J. Madrid-Cuevas, and M. J. Marín-Jiménez. 2014.
 * "Automatic generation and detection of highly reliable fiducial markers under occlusion".
 * Pattern Recogn. 47, 6 (June 2014), 2280-2292. DOI=10.1016/j.patcog.2014.01.005
 */
static int _getSelfDistance(const Mat &marker) {
    Mat bytes = Dictionary::getByteListFromBits(marker);
    int minHamming = (int)marker.total() + 1;
    for(int i = 1; i < 4; i++) {
        int currentHamming = 0;
        for(int j = 0; j < bytes.cols; j++) {
            unsigned char xorRes = bytes.ptr< Vec4b >()[j][0] ^ bytes.ptr< Vec4b >()[j][i];
            currentHamming += hammingWeightLUT[xorRes];
        }
        if(currentHamming < minHamming) minHamming = currentHamming;
    }
    return minHamming;
}

/**
 */
Dictionary generateCustomDictionary(int nMarkers, int markerSize,
                                    const Dictionary &baseDictionary) {

    Dictionary out;
    out.markerSize = markerSize;

    // theoretical maximum intermarker distance
    // See S. Garrido-Jurado, R. Muñoz-Salinas, F. J. Madrid-Cuevas, and M. J. Marín-Jiménez. 2014.
    // "Automatic generation and detection of highly reliable fiducial markers under occlusion".
    // Pattern Recogn. 47, 6 (June 2014), 2280-2292. DOI=10.1016/j.patcog.2014.01.005
    int C = (int)std::floor(float(markerSize * markerSize) / 4.f);
    int tau = 2 * (int)std::floor(float(C) * 4.f / 3.f);

    // if baseDictionary is provided, calculate its intermarker distance
    if(baseDictionary.bytesList.rows > 0) {
        CV_Assert(baseDictionary.markerSize == markerSize);
        out.bytesList = baseDictionary.bytesList.clone();

        int minDistance = markerSize * markerSize + 1;
        for(int i = 0; i < out.bytesList.rows; i++) {
            Mat markerBytes = out.bytesList.rowRange(i, i + 1);
            Mat markerBits = Dictionary::getBitsFromByteList(markerBytes, markerSize);
            minDistance = min(minDistance, _getSelfDistance(markerBits));
            for(int j = i + 1; j < out.bytesList.rows; j++) {
                minDistance = min(minDistance, out.getDistanceToId(markerBits, j));
            }
        }
        tau = minDistance;
    }

    // current best option
    int bestTau = 0;
    Mat bestMarker;

    // after these number of unproductive iterations, the best option is accepted
    const int maxUnproductiveIterations = 5000;
    int unproductiveIterations = 0;

    while(out.bytesList.rows < nMarkers) {
        Mat currentMarker = _generateRandomMarker(markerSize);

        int selfDistance = _getSelfDistance(currentMarker);
        int minDistance = selfDistance;

        // if self distance is better or equal than current best option, calculate distance
        // to previous accepted markers
        if(selfDistance >= bestTau) {
            for(int i = 0; i < out.bytesList.rows; i++) {
                int currentDistance = out.getDistanceToId(currentMarker, i);
                minDistance = min(currentDistance, minDistance);
                if(minDistance <= bestTau) {
                    break;
                }
            }
        }

        // if distance is high enough, accept the marker
        if(minDistance >= tau) {
            unproductiveIterations = 0;
            bestTau = 0;
            Mat bytes = Dictionary::getByteListFromBits(currentMarker);
            out.bytesList.push_back(bytes);
        } else {
            unproductiveIterations++;

            // if distance is not enough, but is better than the current best option
            if(minDistance > bestTau) {
                bestTau = minDistance;
                bestMarker = currentMarker;
            }

            // if number of unproductive iterarions has been reached, accept the current best option
            if(unproductiveIterations == maxUnproductiveIterations) {
                unproductiveIterations = 0;
                tau = bestTau;
                bestTau = 0;
                Mat bytes = Dictionary::getByteListFromBits(bestMarker);
                out.bytesList.push_back(bytes);
            }
        }
    }

    // update the maximum number of correction bits for the generated dictionary
    out.maxCorrectionBits = (tau - 1) / 2;

    return out;
}
}
}
