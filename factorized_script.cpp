#include <string>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
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

//Looks for the duration of the best itinerary, given a JSON provided by OTP


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

//Send a request to OTP to get itineraries from a point A to a point B, given a day and an hour. Possibility to choose departure or arrival mode (TODO)
//RETURNS : JSON with all the possible itineraries

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
  string maxWalkDistance = "2500";
  url += "&maxWalkDistance=" + maxWalkDistance;
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
  string maxWalkDistance = "2500";
  url += "&maxWalkDistance=" + maxWalkDistance;
  url += "&numItineraries=1";

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
  if (argc <9) {
    cout << "Missing information. Usage : ./script <addr> number_of_process process_id departure/arrival MM-DD-YYYY HH:MM am(or pm) count ponctuel/choropleth\n";
    return 0;
  }

  string ip = argv[1];
  string direction = argv[4];
  string date=argv[5];
  string hour=argv[6];
  string ampm = argv[7];

  string mode = argv[9];
  string timestamp=hour+ampm;

  int number_of_process = 5; //number of process to start
  int process_id = *argv[3]- '0';
  int nbr_subset=200; //number of elements in the random subset for computation

  int count=0;

	cout << "Connecting to the OTP server on "<< ip << "\n";
  
  string s;
  vector<string> name;
  vector<string> coord;
  vector<string> code_insee;

  /*Building the datasets regarding the chosen mode 
    Choropleth : Uses LaPoste datasets, will not provide a ponctual map
    Ponctual : Uses stop points (from buses, trains etc), will provide a ponctual map
  */

  if (mode=="choropleth"){

    
    ifstream fin;
    fin.open("laposte_hexasmal.csv");

    
    set<string> knownPostalCodes;
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

        string depCode = values[2].substr(0,2);
        // the original file has \r\n end symbol since it was written on Windows
        string coordClean = values[5].substr(0,values[5].size() - 1); 


        if ((depCode == "94" || depCode == "93" || depCode == "92" || depCode == "75" || depCode == "91" || depCode == "77" || depCode == "95" || depCode == "78")
              &&(knownPostalCodes.find(values[2])==knownPostalCodes.end())) {
          name.push_back(values[1]);
          coord.push_back(coordClean);
          code_insee.push_back(values[0]);
          knownPostalCodes.insert(values[2]);
        }
      }

      count++;
    }
  }

  else if (mode=="ponctuel"){
    ifstream fin;
    fin.open("./stop_ponctuels/stops_test_1000_random.csv"); //Provide correct path
    if (fin.is_open()){
      while (getline(fin, s)) {
      // split, take name and coords
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


          if(values[0]!="index"){      
          // the original file has \r\n end symbol since it was written on Windows
            string coordClean = values[4]; // .substr(0,values[4].size() - 1); 
            coord.push_back(coordClean);
            name.push_back(values[1]);
          
        
          }

        
        }
      
        count++;
      }
      cout << "Output of begin and end: ";
      fin.close();
    }
  }

  else{
    cout<< "Wrong information. Usage : ./script <addr> number_of_process process_id departure/arrival MM-DD-YYYY HH:MM am(or pm) count ponctuel/choropleth\n";
  }
  
  
 




      vector<double> accesibility;
      int N = name.size();
      int shuffle_list[N];

      for(int i = 0; i < N; i++) {
      shuffle_list[i] = i;
      }
      /* 
      Create a random array of int from 0 to N (dataset size that changes regarding the number of processes)
      It allows to reduce computation time by choosing a certain amount of points to use.
      */

      random_shuffle(&shuffle_list[0],&shuffle_list[N]);

      // Accessibility for one coord is computed with nbr_subset other coords (random). Repeat for every coord
      for (int i = 0; i<N; ++i) {
        int total = 0;
        int point_index=0;
        int idx=rand()%((N/2)-1);

        while (point_index< nbr_subset && idx<N){
          int ptr= shuffle_list[idx];
          if (i != ptr) { 
            
            string queryJSON = queryAtoB(coord[ptr], coord[i], ip, direction, date, timestamp);
          
            total += parseTravelTime(queryJSON);
            point_index++;
          }
          cout<< i << "/" << N << endl;
          idx++;
        }
        double avg = 1.0*total / (N - 1);
        accesibility.push_back(avg);
        cout << name[i] << ' ' << coord[i] << ' ' << avg <<" "<<timestamp << '\n';
      }


      //Create folders if they do not exist
      string path=mode;
      mkdir(path.c_str(),0777);

      path+=("/"+to_string(nbr_subset));
      mkdir(path.c_str(),0777);

      path+="/";
      mkdir((path+"csv").c_str(),0777);
      mkdir((path+"map").c_str(),0777);

      /*
      if (mkdir(path.c_str(), 0777) == -1){

          path+="/";
          path+=to_string(nbr_subset);
          if (mkdir(path.c_str(), 0777) == -1){
            cerr << "Error"  << endl;
            mkdir((path+"/csv").c_str(),0777);
            mkdir((path+"/map").c_str(),0777);
          }
        
          else{


            mkdir((path+"/csv").c_str(),0777);
            mkdir((path+"/map").c_str(),0777);

            cout <<path<< "Directory created";
          }

      }
  
      else{
                
          path+="/";
          path+=to_string(nbr_subset);
          if (mkdir(path.c_str(), 0777) == -1)
            cerr << "Error"  << endl;
        
          else


            mkdir((path+"/csv").c_str(),0777);
            mkdir((path+"/map").c_str(),0777);

            cout <<path<< "Directory created";

      }
      */


      //Save data in csv
      cout << " Writing CSV" << endl;

      path+="csv/";
      path+=argv[8];
      path+="access_idf";
      path+=argv[3];
      path+=argv[4];
      path+=".csv";

      
      ofstream fout;
      fout.open(path);
      fout << "Code_INSEE;Name;Coord;Access;time\n";
      for (int i = 0; i<N; ++i) {
        fout << code_insee[i] << ';' << name[i] << ';' << coord[i] << ';' << accesibility[i] << ';' <<date+" "+ hour << '\n';
      }
      fout.close();
    

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << duration.count() << endl;
  return 1;
}
