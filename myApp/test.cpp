#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <fstream>
#include <bits/stdc++.h>

using namespace std;

bool PutObjectBuffer() {
    Aws::Client::ClientConfiguration clientConfig;
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket("nphw3bucket");
    request.SetKey("loginNumber");

    const std::shared_ptr<Aws::IOStream> inputData =
            Aws::MakeShared<Aws::StringStream>("");
    string objectContent = "1 2 3";
    *inputData << objectContent.c_str();

    request.SetBody(inputData);

    Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);

    return outcome.IsSuccess();
}

bool GetObject(const Aws::String &objectKey,
                   const Aws::String &fromBucket) {
	Aws::Client::ClientConfiguration clientConfig;
    Aws::S3::S3Client client(clientConfig);

    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(fromBucket);
    request.SetKey(objectKey);

    Aws::S3::Model::GetObjectOutcome outcome =
            client.GetObject(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error &err = outcome.GetError();
        std::cerr << "Error: GetObject: " <<
                  err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        std::cout << "Successfully retrieved '" << objectKey << "' from '"
                  << fromBucket << "'." << std::endl;
		Aws::IOStream& body = outcome.GetResult().GetBody();	
		string x, y, z; body >> x >> y >> z;
		cout << x << y << z << '\n';
	}

    return outcome.IsSuccess();
}

int main( int argc, char ** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    {
		PutObjectBuffer();
        // make your SDK calls here.
    }

    Aws::ShutdownAPI(options);
    return 0;
}
