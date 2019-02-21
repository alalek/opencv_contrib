// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_QUALITY_QUALITYBRISQUE_HPP
#define OPENCV_QUALITY_QUALITYBRISQUE_HPP

#include "qualitybase.hpp"

namespace cv
{
namespace quality
{

/** @brief Custom deleter for QualityBRISQUE internal data */
struct _QualityBRISQUEDeleter
{
    void operator()(void*) const;
};

/**
@brief TODO:  Brief description and reference to original BRISQUE paper/implementation
*/
class CV_EXPORTS_W QualityBRISQUE : public QualityBase {
public:

    /** @brief Computes BRISQUE quality score for input images
    @param imgs Images for which to compute quality
    @returns Result is a score (averaged over individual scores of all images) ranging from 0 to 100 (0 denotes the best quality and 100 denotes the poorest quality)
    */
    CV_WRAP cv::Scalar compute( InputArrayOfArrays imgs ) CV_OVERRIDE;

    /**
    @brief Create an object which calculates quality
    @param model_file_path cv::String which contains a path to the BRISQUE model data.  If empty, attempts to load from ${OPENCV_DIR}/testdata/contrib/quality/brisque_allmodel.dat
    @param range_file_path cv::String which contains a path to the BRISQUE range data.  If empty, attempts to load from ${OPENCV_DIR}/testdata/contrib/quality/brisque_allrange.dat
    */
    CV_WRAP static Ptr<QualityBRISQUE> create( const cv::String& model_file_path = "", const cv::String& range_file_path = "" );

    /**
    @brief static method for computing quality
    @param imgs image(s) for which to compute quality
    @param model_file_path cv::String which contains a path to the BRISQUE model data.  If empty, attempts to load from ${OPENCV_DIR}/testdata/contrib/quality/brisque_allmodel.dat
    @param range_file_path cv::String which contains a path to the BRISQUE range data.  If empty, attempts to load from ${OPENCV_DIR}/testdata/contrib/quality/brisque_allrange.dat
    @returns 
    */
    CV_WRAP static cv::Scalar compute_single( const Mat& cmpImg, const cv::String& model_file_path, const cv::String& range_file_path );
    CV_WRAP static cv::Scalar compute( InputArrayOfArrays imgs, const cv::String& model_file_path, const cv::String& range_file_path );

protected:

    /** @brief Internal constructor */
    QualityBRISQUE( const cv::String& model_file_path, const cv::String& range_file_path );

    /** @brief Type-erased holder for libsvm data, using custom deleter */
    std::unique_ptr<void, _QualityBRISQUEDeleter> _svm_data;

};  // QualityBRISQUE
}   // quality
}   // cv
#endif
