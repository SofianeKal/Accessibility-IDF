#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <list>
#include <time.h>
#include <set>

#include <jansson.h>
#include <curl/curl.h>

using namespace std;
using namespace std::chrono;

string URL;

static char errorBuffer[CURL_ERROR_SIZE];

// source https://curl.se/libcurl/c/htmltitle.html
static int writer(char *data, size_t size, size_t nmemb,
                  std::string *writerData)
{
  if(writerData == NULL)
    return 0;
 
  writerData->append(data, size*nmemb);
 
  return size * nmemb;
}

static bool init(CURL *&conn, const char *url, std::string &buffer)
{
  CURLcode code;
 
  conn = curl_easy_init();

 
  if(conn == NULL) {
    fprintf(stderr, "Failed to create CURL connection\n");
    exit(EXIT_FAILURE);
  }
 
  code = curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, errorBuffer);
  if(code != CURLE_OK) {
    fprintf(stderr, "Failed to set error buffer [%d]\n", code);
    return false;
  }
 
  code = curl_easy_setopt(conn, CURLOPT_URL, url);
  if(code != CURLE_OK) {
    fprintf(stderr, "Failed to set URL [%s]\n", errorBuffer);
    return false;
  }
 
  code = curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L);
  if(code != CURLE_OK) {
    fprintf(stderr, "Failed to set redirect option [%s]\n", errorBuffer);
    return false;
  }
 
  code = curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, writer);
  if(code != CURLE_OK) {
    fprintf(stderr, "Failed to set writer [%s]\n", errorBuffer);
    return false;
  }
 
  code = curl_easy_setopt(conn, CURLOPT_WRITEDATA, &buffer);
  if(code != CURLE_OK) {
    fprintf(stderr, "Failed to set write data [%s]\n", errorBuffer);
    return false;
  }
 
  return true;
}

static char *request(std::string &url, std::string &buffer) {
  CURL *curl = NULL;
  CURLcode status;

  curl_global_init(CURL_GLOBAL_ALL);

  if (!init(curl, &url[0], buffer)) {
    fprintf(stderr, "Error: curl initialization failed");
  }

  status = curl_easy_perform(curl);

  if(status != CURLE_OK) {
    fprintf(stderr, "Failed to get '%s' [%s]\n", &url[0], errorBuffer);
    exit(EXIT_FAILURE);
  }

  curl_easy_cleanup(curl);
}

long long parseTravelTime(std::string &jsonString) {
  json_t *root;
  json_error_t error;

  root = json_loads(&jsonString[0], 0, &error);

  if (!root) {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    return 1;
  }

  json_t *plan = json_object_get(root, "plan");
  json_t *itineraries = json_object_get(plan, "itineraries");
  json_t *bestItinerarie = json_array_get(itineraries, 0);
  json_t *duration = json_object_get(bestItinerarie, "duration");
  return json_integer_value(duration);
}

string queryAtoB(string coordA, string coordB, string ip, string direction, string day, string timestamp) {

  if(direction=="departure"){

  string buffer;
  string url = "http://" + ip + "/otp/routers/default/plan?";
  string fromCoord = coordA;
  url += "fromPlace=" + fromCoord;
  string toCoord = coordB;
  url += "&toPlace=" + toCoord;
  string date = day;
  url += "&date=" + date;
  string time = timestamp;
  url += "&time=" + time;
  //string maxWalkDistance = "2500";
  //url += "&maxWalkDistance=" + maxWalkDistance;
  url += "&numItineraries=1";
  request(url,buffer);
  return buffer;
  } 

  else if(direction=="arrival"){
  
  string buffer;
  string url = "http://" + ip + "/otp/routers/default/plan?";
  string fromCoord = coordB;
  url += "fromPlace=" + fromCoord;
  string toCoord = coordA;
  url += "&toPlace=" + toCoord;
  string date = day;
  url += "&date=" + date;
  string time = timestamp;
  url += "&time=" + time;
  //string maxWalkDistance = "2500";
  //url += "&maxWalkDistance=" + maxWalkDistance;
  url += "&numItineraries=1";
  url+="&optimize=QUICK";
  url+="&debugItineraryFilter=True&searchWindow=1";

  request(url,buffer);
  return buffer;

  }

  else{
    cout << "4th argument must be either departure or arrival";
    return std::string(0);
  }
}

int main(int argc, char* argv[])
{

  auto start = high_resolution_clock::now(); //To get execution time
  if (argc <8) {
    cout << "Missing information. Usage : ./script <addr> number_of_process process_id departure/arrival MM-DD-YYYY HH:MM am(or pm) count\n";
    return 0;
  }

  string ip = argv[1];
  string direction = argv[4];
  string delimiter ="/";
  string date=argv[5];
  string hour=argv[6];
  string ampm = argv[7];

  string timestamp=hour+ampm;

  int number_of_process = 10;
  int process_id = *argv[3]- '0';
  int elem_sous_ensemble=100;

  int count=0;

	cout << "Connecting to the OTP server on "<< ip << "\n";

  ifstream fin;
  fin.open("grid_europe.csv");
  string s;
  vector<string> name;
  vector<string> coord;
  vector<string> tot_p;
  while (getline(fin, s)) {
    // split, take name, code and coords
    vector<string> values;
    int prev = 0, pos = 0;
    if(count%number_of_process==process_id){

      do {
        pos = s.find(';', prev);
        if (pos == string::npos) pos = s.length();
        string value = s.substr(prev, pos - prev);
        values.push_back(value);
        prev = pos + 1;
      } while (pos < s.length() && prev < s.length());

      name.push_back(values[2]);
      coord.push_back(values[6].substr(0,values[6].size()-1));
      tot_p.push_back(values[1]);
        

      
      
    }

    count++;
  }
  
  fin.close();
 




      vector<double> accesibility;
      int N = name.size();
      int shuffle_list[N];

      for(int i = 0; i < N; i++) {
      shuffle_list[i] = i;
      }

      for (int i = 0; i<N; ++i) {
        int total = 0;
        int point_index=0;
        int idx=rand()%((N/2)-1);

        while (point_index< elem_sous_ensemble && idx<N){
          int ptr= shuffle_list[idx];
          if (i != ptr) { 
            
            string queryJSON = queryAtoB(coord[ptr], coord[i], ip, direction, date, timestamp);
          
            total += parseTravelTime(queryJSON);
            point_index++;
          }
          idx++;
	 cout<< i << "/" << N << endl;
        }
        
        double avg = 1.0*total / (N - 1);
        accesibility.push_back(avg);
        cout << name[i] << ' ' << coord[i] << ' ' << avg << " " <<timestamp << '\n';
      }

      cout << " Writing CSV" << endl;
      string strcsv="grid/csv/";
      strcsv+=argv[8];
      strcsv+="access_idf";
      strcsv+=argv[3];
      strcsv+=argv[4];
      strcsv+=".csv";

      
      ofstream fout;
      fout.open(strcsv);

      fout << "Grid_ID;Coord;Access;Tot. population met;time\n";
      for (int i = 0; i<N; ++i) {
        fout  << name[i] << ';' << coord[i] << ';' << accesibility[i] <<  ";" << date+" "+ hour << '\n';
      }
      fout.close();
    
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << duration.count() << endl;

  return 1;
}
