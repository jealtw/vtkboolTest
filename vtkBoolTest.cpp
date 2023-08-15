#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProperty.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataBooleanFilter.h>
#include <vtkDataObject.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkTriangleFilter.h>
#include <vtkCleanPolyData.h>


vtkSmartPointer<vtkPolyData> ReadSTLData(const char* fileFullPathName)
{
	vtkNew<vtkSTLReader> stlReader;
	stlReader->SetFileName(fileFullPathName);
	stlReader->Update();
	vtkSmartPointer<vtkPolyData> data = stlReader->GetOutput();
	return data;
}

void SavePolyData(const char* fileFullPathName, vtkDataObject* data)
{
	vtkNew<vtkSTLWriter> stlWriter;
	stlWriter->SetFileName(fileFullPathName);
	stlWriter->SetInputData(data);
	int writerResult = stlWriter->Write();
	if (writerResult)
		std::cout << "SavePolyData " << fileFullPathName << " Succeed!" << std::endl;
	else
	{
		std::cout << "SavePolyData " << fileFullPathName << " Failed!" << std::endl;
	}
}

vtkSmartPointer<vtkPolyData> PreprocessData(vtkSmartPointer<vtkPolyData> inputData)
{
	vtkNew<vtkPolyDataNormals> normals;
	normals->SetInputData(inputData);
	normals->ComputePointNormalsOn();
	normals->ComputeCellNormalsOff();
	normals->FlipNormalsOff();
	normals->SplittingOff();
	normals->ConsistencyOn();
	normals->AutoOrientNormalsOff();
	normals->Update();

	vtkNew<vtkSmoothPolyDataFilter> smoothFilter;
	smoothFilter->SetInputConnection(normals->GetOutputPort());
	smoothFilter->Update();

	vtkNew<vtkCleanPolyData> clean;
	clean->SetInputConnection(smoothFilter->GetOutputPort());
	clean->ConvertStripsToPolysOn();
	clean->ConvertPolysToLinesOn();
	clean->ConvertLinesToPointsOn();
	//clean->PointMergingOff();
	clean->Update();

	vtkNew<vtkTriangleFilter> triangleFilter;
	triangleFilter->SetInputConnection(clean->GetOutputPort());
	triangleFilter->PassLinesOff();
	triangleFilter->PassVertsOff();
	triangleFilter->Update();

	vtkNew<vtkCleanPolyData> cleanAgain;
	cleanAgain->SetInputConnection(triangleFilter->GetOutputPort());
	cleanAgain->ConvertStripsToPolysOn();
	cleanAgain->ConvertPolysToLinesOn();
	cleanAgain->ConvertLinesToPointsOn();
	//cleanAgain->PointMergingOff();
	cleanAgain->Update();

	vtkNew<vtkPolyDataNormals> normalsAgain;
	normalsAgain->SetInputConnection(cleanAgain->GetOutputPort());
	normalsAgain->ComputePointNormalsOn();
	normalsAgain->ComputeCellNormalsOff();
	normalsAgain->FlipNormalsOff();
	normalsAgain->SplittingOff();
	normalsAgain->ConsistencyOn();
	normalsAgain->AutoOrientNormalsOff();
	normalsAgain->Update();

	vtkSmartPointer<vtkPolyData> outputData = normalsAgain->GetOutput();

	return outputData;
}

void TestvtkBoolDifference(const char* data0File, const char* data1File, const char* boolResultFile, bool shouldPreprocessDatas = false)
{
	std::cout << data0File << " Difference " << data1File << std::endl;

	vtkSmartPointer<vtkPolyData> data0 = ReadSTLData(data0File);
	vtkSmartPointer<vtkPolyData> data1 = ReadSTLData(data1File);

	if (shouldPreprocessDatas)
	{
		data0 = PreprocessData(data0);
		data1 = PreprocessData(data1);		
	}

	vtkNew<vtkPolyDataBooleanFilter> polyDataBooleanFilter;	
	polyDataBooleanFilter->SetInputData(0, data0);
	polyDataBooleanFilter->SetInputData(1, data1);
	polyDataBooleanFilter->SetOperModeToDifference();

	polyDataBooleanFilter->Update();
	vtkSmartPointer<vtkPolyData> result = polyDataBooleanFilter->GetOutput();
	SavePolyData(boolResultFile, result);
}

void TestvtkBoolUnion(const char* data0File, const char* data1File, const char* boolResultFile, bool shouldPreprocessDatas = false)
{
	std::cout << data0File << " Union " << data1File << std::endl;

	vtkSmartPointer<vtkPolyData> data0 = ReadSTLData(data0File);
	vtkSmartPointer<vtkPolyData> data1 = ReadSTLData(data1File);

	if (shouldPreprocessDatas)
	{
		data0 = PreprocessData(data0);
		data1 = PreprocessData(data1);
	}

	vtkNew<vtkPolyDataBooleanFilter> polyDataBooleanFilter;
	polyDataBooleanFilter->SetInputData(0, data0);
	polyDataBooleanFilter->SetInputData(1, data1);
	polyDataBooleanFilter->SetOperModeToUnion();

	polyDataBooleanFilter->Update();
	vtkSmartPointer<vtkPolyData> result = polyDataBooleanFilter->GetOutput();
	SavePolyData(boolResultFile, result);
}

int main(int argc, char** argv)
{
	//Test Case 1 : Union Test, but the result seems to be the result of Difference instead of Union
	TestvtkBoolUnion("Data0-Union.stl", "Data1-Union.stl", "Data0-Union-Data1.stl");

	//Test Case 2 : May Crash,  or get the result of Union instead of Difference Under x64 Debug Mode
	TestvtkBoolDifference("Data0-Union.stl", "Data1-Union.stl", "Data0-Difference-Data1.stl");

	////Test Case 3 : Error with message "Contact ends suddenly."
	//TestvtkBoolDifference("Data3-Crash.stl", "Data4-Crash.stl", "Data3-Difference-Data4.stl", true);

	////Test Case 4 : May Crash,  or get the result of Union instead of Difference Under x64 Debug Mode
	//TestvtkBoolDifference("Data0-Union.stl", "Data4-Crash.stl", "Data0-Difference-Data4.stl");

	////Test Case 5 : Error with message "Contact ends suddenly."
	//TestvtkBoolDifference("Data3-Crash.stl", "Data1-Union.stl", "Data3-Difference-Data1.stl");


	std::cin.get();


	return 0;
}
