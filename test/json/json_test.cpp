#include <cstdio>
#include <cstring>

// This is the JsonCpp header
// g++ -o test_json json_test.cpp /usr/lib/libjson_linux-gcc-4.6.1_libmt.so
#include "jsoncpp/json.h"

using namespace std;

int main(int argc, char **argv)
{
    string json_example = "{\"array\":[\"item1\", \"item2\"], \"not an array\":\"asdf\"}";


    // Let's parse it    
    Json::Value root;
    Json::Reader reader;
    
    bool parsedSuccess = reader.parse(json_example, root, false);
    
    if ( !parsedSuccess )
    {
        // report to the user the failure and their locations in the document.
        cout  << "Failed to parse JSON"<< endl << reader.getFormatedErrorMessages()<< endl;
        return 1;
    }
    
    // Let's extract the array that is contained inside the root object
    const Json::Value array = root["array"];
    
    // And print its values
    for ( int index = 0; index < array.size(); ++index )  // Iterates over the sequence elements.
        cout<<"Element " << index <<" in array: "<< array[index].asString() << endl;

    // Lets extract the not array element contained in the root object and print its value
    cout<<"Not an array: "<<root["not an array"]<<endl;

    // If we want to print JSON is as easy as doing:
    cout << "Json Example pretty print: " <<endl<< root.toStyledString() << endl;
    
    return 0;
}
