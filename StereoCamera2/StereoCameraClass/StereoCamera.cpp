#include "StereoCamera.h"

void StereoCamera::saveCloudPoints(std::string cloud_point_filename, std::string distance_filename, const cv::Mat& mat)
{
	const double max_z = 1.0e4;
	std::ofstream fp(cloud_point_filename);
	std::ofstream fpDistance(distance_filename);
	if (!fp.is_open())
	{
		std::cout<<"�򿪵����ļ�ʧ��"<<std::endl;    
		fp.close();  
		return ;
	}  
	//����д��
	for(int y = 0; y < mat.rows; y++)
	{
		for(int x = 0; x < mat.cols; x++)
		{
			cv::Vec3f point = mat.at<cv::Vec3f>(y, x);   //��ͨ��������
			if(fabs(point[2] - max_z) < FLT_EPSILON || fabs(point[2]) > max_z)   
				continue;   
			fp<<point[0]<<" "<<point[1]<<" "<<point[2]<<std::endl;
			fpDistance<<sqrt(point[0]*point[0] + point[1]*point[1] + point[2]+point[2])<<std::endl; 
		}
	}
	fp.close();
	fpDistance.close();
}

void StereoCamera::initPictureFileList(std::string pic_filename, int first, int last)
{
	picFileList.clear();
	for(int cur = first; cur <= last; cur++){
		std::string str_file = std::string(pic_filename) + "\\" + std::to_string(cur) + ".jpg";
		std::cout<<str_file<<std::endl;
		picFileList.push_back(str_file);
	}
}

void StereoCamera::saveParallaxPic(std::string filename, const cv::Mat& mat)
{
	std::ofstream fp(filename, std::ios::out);
	fp<<mat.rows<<std::endl;
	fp<<mat.cols<<std::endl;
	for(int y = 0; y < mat.rows; y++)
	{
		for(int x = 0; x < mat.cols; x++)
		{
			double disp = mat.at<short>(y, x); // �����Ӳ������CV_16S ��ʽ�ģ����� short ���Ͷ�ȡ
			fp<<disp<<std::endl;       // ���Ӳ������ CV_32F ��ʽ������ float ���Ͷ�ȡ
		}
	}
	fp.close();
}

cv::Mat StereoCamera::mergeImg(cv::Mat img1, cv::Mat disp8){
	cv::Mat color_mat = cv::Mat::zeros(img1.size(), CV_8UC3);

	cv::Mat red = img1.clone();
	cv::Mat green = disp8.clone();
	cv::Mat blue = cv::Mat::zeros(img1.size(), CV_8UC1);

	std::vector<cv::Mat> vec;
	vec.push_back(red);
	vec.push_back(blue);
	vec.push_back(green);
	cv::merge(vec, color_mat);
	return color_mat;
}

void StereoCamera::fromGrayToColor(cv::Mat gray_mat, cv::Mat& color_mat)
{
	using namespace cv;
	color_mat = Mat::zeros(gray_mat.size(), CV_8UC3);
	int rows = color_mat.rows, cols = color_mat.cols;

	Mat red = Mat(gray_mat.rows, gray_mat.cols, CV_8U);
	Mat green = Mat(gray_mat.rows, gray_mat.cols, CV_8U);
	Mat blue = Mat(gray_mat.rows, gray_mat.cols, CV_8U);
	Mat mask = Mat(gray_mat.rows, gray_mat.cols, CV_8U);

	subtract(gray_mat, Scalar(255), blue);         // blue(I) = 255 - gray(I)
	red = gray_mat.clone();                        // red(I) = gray(I)
	green = gray_mat.clone();                      // green(I) = gray(I),if gray(I) < 128

	compare(green, 128, mask, CMP_GE);             // green(I) = 255 - gray(I), if gray(I) >= 128
	subtract(green, Scalar(255), green, mask);
	convertScaleAbs(green, green, 2.0, 2.0);

	vector<Mat> vec;
	vec.push_back(red);
	vec.push_back(green);
	vec.push_back(blue);
	cv::merge(vec, color_mat);
}

void StereoCamera::stereoCalibrateCamera(std::string intrinsic_filename, std::string extrinsic_filename)
{
	using namespace cv;
	std::vector<int> idx;
	//�������Ľǵ�������Ҳ�����Ľǵ�����
	std::vector<std::vector<cv::Point2f>> imagePoints[2];

	//vector<vector<Point2f>> leftPtsList(fileList.size());
	//vector<vector<Point2f>> rightPtsList(fileList.size());

	for(uint i = 0; i < picFileList.size();++i)
	{
		std::vector<cv::Point2f> leftPts, rightPts;      // �洢��������Ľǵ�λ��
		cv::Mat rawImg = cv::imread(picFileList[i]);					   //ԭʼͼ��
		if(rawImg.empty()){
			std::cout<<"the Image is empty..."<<picFileList[i]<<std::endl;
			continue;
		}
		//��ȡ����ͼƬ
		Rect leftRect(0, 0, imgSize.width, imgSize.height);
		Rect rightRect(imgSize.width, 0, imgSize.width, imgSize.height);

		Mat leftRawImg = rawImg(leftRect);       //�зֵõ�����ԭʼͼ��
		Mat rightRawImg = rawImg(rightRect);     //�зֵõ�����ԭʼͼ��

		//imwrite("left.jpg", leftRawImg);
		//imwrite("right.jpg", rightRawImg);
		//std::cout<<"���ͼ��  ���"<<leftRawImg.size().width<<"  �߶�"<<rightRawImg.size().height<<endl;
		//std::cout<<"�Ҳ�ͼ��  ���"<<rightRawImg.size().width<<"  �߶�"<<rightRawImg.size().height<<endl;  //

		Mat leftImg, rightImg, leftSimg, rightSimg, leftCimg, rightCimg, leftMask, rightMask;
		// BGT -> GRAY	
		if(leftRawImg.type() == CV_8UC3)
			cvtColor(leftRawImg, leftImg, CV_BGR2GRAY);  //תΪ�Ҷ�ͼ
		else
			leftImg = leftRawImg.clone();
		if(rightRawImg.type() == CV_8UC3)
			cvtColor(rightRawImg, rightImg, CV_BGR2GRAY); 
		else
			rightImg = rightRawImg.clone();

		imgSize = leftImg.size();

		//ͼ���˲�Ԥ����
		resize(leftImg, leftMask, Size(200, 200));		//resize��ԭͼ��img���µ�����С����maskͼ���СΪ200*200
		resize(rightImg, rightMask, Size(200, 200));
		GaussianBlur(leftMask, leftMask, Size(13, 13), 7);   
		GaussianBlur(rightMask, rightMask, Size(13, 13), 7); 
		resize(leftMask, leftMask, imgSize);
		resize(rightMask, rightMask, imgSize);
		medianBlur(leftMask, leftMask, 9);    //��ֵ�˲�
		medianBlur(rightMask, rightMask, 9);

		for (int v = 0; v < imgSize.height; v++) {
			for (int u = 0; u < imgSize.width; u++) {
				int leftX = ((int)leftImg.at<uchar>(v, u) - (int)leftMask.at<uchar>(v, u)) * 2 + 128;
				int rightX = ((int)rightImg.at<uchar>(v, u) - (int)rightMask.at<uchar>(v, u)) * 2 + 128;
				leftImg.at<uchar>(v, u)  = max(min(leftX, 255), 0);
				rightImg.at<uchar>(v, u) = max(min(rightX, 255), 0);
			}
		}

		//Ѱ�ҽǵ㣬 ͼ������
		resize(leftImg, leftSimg, Size(), imgScale, imgScale);      //ͼ����0.5�ı�������
		resize(rightImg, rightSimg, Size(), imgScale, imgScale);
		cvtColor(leftSimg, leftCimg, CV_GRAY2BGR);     //תΪBGRͼ��cimg��simg��800*600��ͼ��
		cvtColor(rightSimg, rightCimg, CV_GRAY2BGR);


		//Ѱ�����̽ǵ�
		bool leftFound = findChessboardCorners(leftCimg, chessboardSize, leftPts, CV_CALIB_CB_ADAPTIVE_THRESH|CV_CALIB_CB_FILTER_QUADS);
		bool rightFound = findChessboardCorners(rightCimg, chessboardSize, rightPts, CV_CALIB_CB_ADAPTIVE_THRESH|CV_CALIB_CB_FILTER_QUADS);

		if(leftFound)
			cornerSubPix(leftSimg, leftPts, Size(11, 11), Size(-1,-1 ), 
			TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 300, 0.01));
		if(rightFound)
			cornerSubPix(rightSimg, rightPts, Size(11, 11), Size(-1, -1), 
			TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 300, 0.01));   //������

		//�Ŵ�Ϊԭ���ĳ߶�
		for(uint j = 0;j < leftPts.size();j++) //�÷�ͼ��132���ǵ㣬�������2����ԭ�ǵ�λ��
			leftPts[j] *= 1./imgScale;
		for(uint j = 0;j < rightPts.size();j++)
			rightPts[j] *= 1./imgScale;

		//��ʾ
		string leftWindowName = "Left Corner Pic", rightWindowName = "Right Corner Pic";

		Mat leftPtsTmp = Mat(leftPts) * imgScale;      //�ٴγ��� imgScale
		Mat rightPtsTmp = Mat(rightPts) * imgScale;

		drawChessboardCorners(leftCimg, chessboardSize, leftPtsTmp, leftFound);      //���ƽǵ����겢��ʾ
		imshow(leftWindowName, leftCimg);
		imwrite("���/DrawChessBoard/"+std::to_string(i)+"_left.jpg", leftCimg);
		waitKey(200);

		drawChessboardCorners(rightCimg, chessboardSize, rightPtsTmp, rightFound);   //���ƽǵ����겢��ʾ
		imshow(rightWindowName, rightCimg);
		imwrite("���/DrawChessBoard/"+std::to_string(i)+"_right.jpg", rightCimg);
		waitKey(200);

		cv::destroyAllWindows();

		//����ǵ�����
		if(leftFound && rightFound)   
		{
			imagePoints[0].push_back(leftPts);
			imagePoints[1].push_back(rightPts);  //����ǵ�����
			std::cout<<"ͼƬ "<<i<<" ����ɹ���"<<std::endl;
			idx.push_back(i);
		}
	}
	cv::destroyAllWindows();
	imagePoints[0].resize(idx.size());
	imagePoints[1].resize(idx.size());
	std::cout<<"�ɹ��궨�ı궨�����Ϊ"<<idx.size()<<"  ��ŷֱ�Ϊ: ";
	for(unsigned int i = 0;i < idx.size();++i)
		std::cout<<idx[i]<<"  ";

	//�����������
	vector<vector<Point3f>> objPts(idx.size());  //idx.size����ɹ�����ͼ��ĸ���
	for (int y = 0; y < chessboardSize.height; y++) {
		for (int x = 0; x < chessboardSize.width; x++) {
			objPts[0].push_back(Point3f((float)x, (float)y, 0) * chessboardLength);
		}
	}
	for (uint i = 1; i < objPts.size(); i++) {
		objPts[i] = objPts[0];
	}

	//
	// ˫Ŀ����궨
	Mat cameraMatrix[2], distCoeffs[2];
	vector<Mat> rvecs[2], tvecs[2]; 
	cameraMatrix[0] = Mat::eye(3, 3, CV_64F);
	cameraMatrix[1] = Mat::eye(3, 3, CV_64F);
	Mat R, T, E, F;

	cv::calibrateCamera(objPts, imagePoints[0], imgSize, cameraMatrix[0],    
		distCoeffs[0], rvecs[0], tvecs[0], CV_CALIB_FIX_K3);

	cv::calibrateCamera(objPts, imagePoints[1], imgSize, cameraMatrix[1],    
		distCoeffs[1], rvecs[1], tvecs[1], CV_CALIB_FIX_K3);

	std::cout<<"Left Camera Matrix: "<<std::endl<<cameraMatrix[0]<<std::endl;
	std::cout<<"Right Camera Matrix��"<<std::endl<<cameraMatrix[1]<<std::endl;
	std::cout<<"Left Camera DistCoeffs: "<<std::endl<<distCoeffs[0]<<std::endl;
	std::cout<<"Right Camera DistCoeffs: "<<std::endl<<distCoeffs[1]<<std::endl;

	double rms = stereoCalibrate(objPts, imagePoints[0], imagePoints[1],
		cameraMatrix[0], distCoeffs[0],
		cameraMatrix[1], distCoeffs[1],
		imgSize, R, T, E, F,
		TermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 100, 1e-5));
	//CV_CALIB_USE_INTRINSIC_GUESS);


	std::cout<<"����궨��ɣ� "<<std::endl<<"done with RMS error=" << rms << std::endl;  //����ͶӰ���
	std::cout<<"Left Camera Matrix: "<<std::endl<<cameraMatrix[0]<<std::endl;
	std::cout<<"Right Camera Matrix��"<<std::endl<<cameraMatrix[1]<<std::endl;
	std::cout<<"Left Camera DistCoeffs: "<<std::endl<<distCoeffs[0]<<std::endl;
	std::cout<<"Right Camera DistCoeffs: "<<std::endl<<distCoeffs[1]<<std::endl;


	// �궨���ȼ��
	// ͨ�����ͼ���ϵ�����һ��ͼ��ļ��ߵľ��������۱궨�ľ��ȡ�Ϊ��ʵ�����Ŀ�ģ�ʹ�� undistortPoints ����ԭʼ����ȥ����Ĵ���
	// ���ʹ�� computeCorrespondEpilines �����㼫�ߣ��������ߵĵ�����ۼƵľ�������γ������
	std::cout<<" ���߼���...  ������... "<<std::endl;
	double err = 0;
	int npoints = 0;
	vector<Vec3f> lines[2];
	for(unsigned int i = 0; i < idx.size(); i++)
	{
		int npt = (int)imagePoints[0][i].size();  //�ǵ����
		Mat imgpt[2];
		for(int k = 0; k < 2; k++ )
		{
			imgpt[k] = Mat(imagePoints[k][i]);  //
			undistortPoints(imgpt[k], imgpt[k], cameraMatrix[k], distCoeffs[k], Mat(), cameraMatrix[k]); // ����
			computeCorrespondEpilines(imgpt[k], k+1, F, lines[k]);  // ���㼫��
		}
		for(int j = 0; j < npt; j++ )
		{
			double errij = fabs(imagePoints[0][i][j].x*lines[1][j][0] +
				imagePoints[0][i][j].y*lines[1][j][1] + lines[1][j][2]) +
				fabs(imagePoints[1][i][j].x*lines[0][j][0] +
				imagePoints[1][i][j].y*lines[0][j][1] + lines[0][j][2]);
			err += errij;   // �ۼ����
		}
		npoints += npt;
	}
	std::cout << "  ƽ����� average reprojection err = " <<  err/npoints << std::endl;  // ƽ�����

	// ����ڲ����ͻ���ϵ��д���ļ�
	FileStorage fs(intrinsic_filename, CV_STORAGE_WRITE);
	if(fs.isOpened())
	{
		fs << "M1" << cameraMatrix[0] << "D1" << distCoeffs[0] <<
			"M2" << cameraMatrix[1] << "D2" << distCoeffs[1];
		fs.release();
	}
	else
		std::cout << "Error: can not save the intrinsic parameters\n";

	// �������  BOUGUET'S METHOD
	Mat R1, R2, P1, P2, Q;
	Rect validRoi[2];

	cv::stereoRectify(cameraMatrix[0], distCoeffs[0],
		cameraMatrix[1], distCoeffs[1],
		imgSize, R, T, R1, R2, P1, P2, Q,
		CALIB_ZERO_DISPARITY, 1, imgSize, &validRoi[0], &validRoi[1]);

	fs.open(extrinsic_filename, CV_STORAGE_WRITE);
	if( fs.isOpened() )
	{
		fs << "R" << R << "T" << T << "R1" << R1 << "R2" << R2 << "P1" << P1 << "P2" << P2 << "Q" << Q;
		fs.release();
	}
	else
		std::cout << "Error: can not save the intrinsic parameters\n";

	std::cout<<"˫Ŀ�궨���..."<<std::endl;
}

void StereoCamera::stereoMatch(int pic_num, std::string intrinsic_filename, 
		             std::string extrinsic_filename, bool no_display, std::string point_cloud_filename, std::string distance_filename)
{
	using namespace cv;
	int color_mode = 0;
	Mat rawImg = imread(picFileList[pic_num], color_mode);    //������ͼ��  grayScale
	if(rawImg.empty()){
		std::cout<<"In Function stereoMatch, the Image is empty..."<<std::endl;
		exit(-1);
	}
	//��ȡ
	int64 time_t  = getTickCount();
	Rect leftRect(0, 0, imgSize.width, imgSize.height);
	Rect rightRect(imgSize.width, 0, imgSize.width, imgSize.height);
	Mat img1 = rawImg(leftRect);       //�зֵõ�����ԭʼͼ��
	Mat img2 = rawImg(rightRect);      //�зֵõ�����ԭʼͼ��
	time_t = getTickCount() - time_t;
	printf("�ü�ͼ���ʱ��%fms\n",time_t*1000/getTickFrequency());
	//ͼ����ݱ�������
	if(imgScale != 1.f){
		time_t = getTickCount();
		Mat temp1, temp2;
		int method = imgScale < 1 ? INTER_AREA : INTER_CUBIC;
		resize(img1, temp1, Size(), imgScale, imgScale, method);
		img1 = temp1;
		resize(img2, temp2, Size(), imgScale, imgScale, method);
		img2 = temp2;
		time_t = getTickCount() - time_t;
		printf("����ͼ���ʱ��%fms\n", time_t*1000/getTickFrequency());
	}
	imwrite("���/ԭʼ��ͼ��.jpg", img1);
	imwrite("���/ԭʼ��ͼ��.jpg", img2);

	Size img_size = img1.size();

	// reading intrinsic parameters
	FileStorage fs(intrinsic_filename, CV_STORAGE_READ);
	if(!fs.isOpened())
	{
		std::cout<<"Failed to open file "<<intrinsic_filename<<std::endl;
		exit(0);
	}
	Mat M1, D1, M2, D2;      //����������ڲ�������ͻ���ϵ��
	fs["M1"] >> M1;
	fs["D1"] >> D1;
	fs["M2"] >> M2;
	fs["D2"] >> D2;

	M1 *= imgScale;
	M2 *= imgScale;
	time_t = getTickCount();
	// ��ȡ˫Ŀ����������������
	fs.open(std::string(extrinsic_filename), CV_STORAGE_READ);
	if(!fs.isOpened())
	{
		std::cout<<"Failed to open file  "<<std::string(extrinsic_filename)<<std::endl;
		exit(1);
	}

	// �������
	Rect roi1, roi2;
	Mat Q;
	Mat R, T, R1, P1, R2, P2;
	fs["R"] >> R;
	fs["T"] >> T;

	//AlphaȡֵΪ-1ʱ��OpenCV�Զ��������ź�ƽ��
	cv::stereoRectify( M1, D1, M2, D2, img_size, R, T, R1, R2, P1, P2, Q, CALIB_ZERO_DISPARITY, -1, img_size, &roi1, &roi2 );

	// ��ȡ������Ľ���ӳ��
	Mat map11, map12, map21, map22;
	initUndistortRectifyMap(M1, D1, R1, P1, img_size, CV_16SC2, map11, map12);
	initUndistortRectifyMap(M2, D2, R2, P2, img_size, CV_16SC2, map21, map22);

	// ����ԭʼͼ��
	Mat img1r, img2r;
	remap(img1, img1r, map11, map12, INTER_LINEAR);
	remap(img2, img2r, map21, map22, INTER_LINEAR);
	img1 = img1r;
	img2 = img2r;
	time_t = getTickCount() - time_t;
	printf("����У����ʱ��%fms\n",time_t*1000/getTickFrequency());
	// ��ʼ�� stereoBMstate �ṹ��
	int64 t = getTickCount();
	StereoBM bm;

	int unitDisparity = 15;//40
	int numberOfDisparities = unitDisparity * 16;
	bm.state->roi1 = roi1;
	bm.state->roi2 = roi2;
	bm.state->preFilterCap = 13;
	bm.state->SADWindowSize = 19;                                     // ���ڴ�С
	bm.state->minDisparity = 0;                                       // ȷ��ƥ�����������￪ʼ  Ĭ��ֵ��0
	bm.state->numberOfDisparities = numberOfDisparities;                // �ڸ���ֵȷ�����ӲΧ�ڽ�������
	bm.state->textureThreshold = 1000;//10                                  // ��֤���㹻�������Կ˷�����
	bm.state->uniquenessRatio = 1;     //10                               // !!ʹ��ƥ�书��ģʽ
	bm.state->speckleWindowSize = 200;   //13                             // ����Ӳ���ͨ����仯�ȵĴ��ڴ�С, ֵΪ 0 ʱȡ�� speckle ���
	bm.state->speckleRange = 32;    //32                                  // �Ӳ�仯��ֵ�����������Ӳ�仯������ֵʱ���ô����ڵ��Ӳ����㣬int ��
	bm.state->disp12MaxDiff = -1;

	// ����
	Mat disp, disp8;
	
	bm(img1, img2, disp);
	t = getTickCount() - t;
	printf("BM����ƥ���ʱ: %fms\n", t*1000/getTickFrequency());

	// ��16λ�������ε��Ӳ����ת��Ϊ8λ�޷������ξ���
	disp.convertTo(disp8, CV_8U, 255/(numberOfDisparities*16.));

	// �Ӳ�ͼתΪ��ɫͼ
	Mat vdispRGB = disp8;
	fromGrayToColor(disp8, vdispRGB);
	// ��������ͼ�����Ӳ�ͼ�ں�
	Mat merge_mat = mergeImg(img1, disp8);

	saveParallaxPic("���/�Ӳ�����.txt", disp);

	//��ʾ
	if(!no_display){
		time_t = getTickCount();
		imshow("������ͼ��", img1);
		imwrite("���/left_undistortRectify.jpg", img1);
		imshow("�Ҳ����ͼ��", img2);
		imwrite("���/right_undistortRectify.jpg", img2);
		imshow("�Ӳ�ͼ", disp8);
		imwrite("���/�Ӳ�ͼ.jpg", disp8);
		imshow("�Ӳ�ͼ_��ɫ.jpg", vdispRGB);
		imwrite("���/�Ӳ�ͼ_��ɫ.jpg", vdispRGB);
		imshow("�����ͼ�����Ӳ�ͼ�ϲ�ͼ��", merge_mat);
		imwrite("���/�����ͼ�����Ӳ�ͼ�ϲ�ͼ��.jpg", merge_mat);
		time_t = getTickCount() - time_t;
		printf("��ʾ��ʱ��%fms",time_t*1000/getTickFrequency());
		cv::waitKey();
		std::cout<<std::endl;
	}
	cv::destroyAllWindows();

	// �Ӳ�ͼתΪ���ͼ
	std::cout<<"�������ӳ��... "<<std::endl;
	Mat xyz;
	reprojectImageTo3D(disp, xyz, Q, true);    //������ͼ  disp: 720*1280 
	cv::destroyAllWindows(); 
	std::cout<<"�����������... "<<std::endl;
	saveCloudPoints(point_cloud_filename, distance_filename, xyz);

	std::cout<<std::endl<<"����"<<std::endl<<"Press any key to end... ";

	getchar(); 
}