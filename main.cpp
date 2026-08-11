#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <unordered_map>

using namespace std ;

#define endl '\n'

#define int long long 

const double TIME_LIMIT = 110.0 ; //in sec's

// some predefined parameters that we use 
unordered_map<string,double>trigrams;
unordered_map<string,double>quadgrams;

double net_val_t ;
double net_val_q ;
//to store the unique char's that we came through
set<char>cipher_set;

bool isallowed(char ch){
    if(ch == ' ' || ch == ',' || ch=='.' || ch==';' ||ch=='!')return false;
    return true ;
}

bool is_space(char ch){
    if(ch==' ')return true ;
    return false ;
}

//purpoose of this funcn : to get the CT after remobing the unwanted stuff ,along with storing the words and checking the uniqueness(i.e how many unique chars came)
string get_cipher(string str , vector<string>&cipher_words){
    string final_Str = "";
    int n = str.size();
    string word="";
    for(int i=0 ; i<n ; i++){
        if(isallowed(str[i])){
            cipher_set.insert(str[i]);
            final_Str+=str[i] ;
            word+=str[i];
        }else{
            if(word!="")cipher_words.push_back(word);  
            word="";
        }
    }
    if(word!="")cipher_words.push_back(word);
    return final_Str ;
}

void load_trigrams(const string& filename){
    ifstream file(filename);

    int net_count = 0 ;
    string key ;
    int val ;

    while(file>>key>>val){
        trigrams[key]=val ;
        net_count+=val ; 
    }
    file.close();

    //now we need some scoring procedure ,and since the net_count would be too large , use the probabilities 
    for(auto& it:trigrams){ 
        double prob = (double)it.second/net_count;
        //taking the log  
        it.second = log10(prob);
    }

    net_val_t = log10(0.01/net_count);
    // cout<<"successfully loaded and created the scorings from the trigrams.txt"<<endl;
}

void load_quadgrams(const string& filename){
    ifstream file(filename);

    int net_count = 0 ;
    string key ;
    int val ;

    while(file>>key>>val){
        quadgrams[key]=val ;
        net_count+=val ; 
    }
    file.close();

    //now we need some scoring procedure ,and since the net_count would be too large , use the probabilities 
    for(auto& it:quadgrams){  
        double prob = (double)it.second/net_count;
        //take the log now 
        it.second = log10(prob);
    }

    net_val_q = log10(0.01/net_count);
    // cout<<"successfully loaded and created the scorings from the quadgrams.txt"<<endl;
}

string randomize_key(string &str){
    string new_str = str ;
    int x = rand()%str.size();
    int y = rand()%str.size();
    swap(new_str[x],new_str[y]);
    return new_str ;
}

double fit_score(string word){
    double score = 0 ;
    //we would basically try to extracts the trigrams from the word and then add the score that we stored in the trigrams map created earlier 
    int n = word.size();
    
    //for trigrams 
    for(int i=0 ; i<n-2 ;i++){
        string tri="" ;
        for(int j=0 ; j<3 ; j++){
             tri += toupper(word[i+j]);
        }
        if(trigrams.find(tri) != trigrams.end()){
            score += trigrams[tri];
        }else{
            score += net_val_t ; 
        }
    }

    //for quadgrams 
    for(int i=0 ; i<n-3 ;i++){
        string quad="" ;
        for(int j=0 ; j<4 ; j++){
             quad += toupper(word[i+j]);
        }

        if(quadgrams.find(quad) != quadgrams.end()){
            score += quadgrams[quad];
        }else{
            score += net_val_q ; 
        }
    }
    return score ; 
}

double calculate_score(vector<char>&cipher_chars , string &key ,vector<string>&cipher_words){
    //create the mapping of the cipher characters to the current key 
    map<char,char>char_map ;
    for(int i=0 ; i<key.size() && i<cipher_chars.size() ; i++){
        char ciph_ch = cipher_chars[i];
        char_map[ciph_ch] = key[i];
    }

    double grade = 0.0 ;
    for(int i=0 ; i<cipher_words.size() ;i++){
        string decipher_word;
        for(int j =0 ; j<cipher_words[i].size();j++){
            if(char_map.find(cipher_words[i][j]) != char_map.end()){
                decipher_word += char_map[cipher_words[i][j]];
            }
        }
        grade += fit_score(decipher_word);
    }
    return grade ;
}

string decrypt(string key , vector<char>& cipher_chars , vector<string>& cipher_words){
    map<char,char>d_map ;
    for(int i=0 ; i<key.size() && i<cipher_chars.size() ; i++){
        char ciph_ch = cipher_chars[i];
        d_map[ciph_ch] = key[i];
    }

    string msg;
    for(int i=0 ; i<cipher_words.size() ;i++){
        string decipher_word;
        if(i!=0)msg+=" " ;
        for(int j =0 ; j<cipher_words[i].size() ;j++){
            if(d_map.find(cipher_words[i][j]) != d_map.end()){
                msg += d_map[cipher_words[i][j]];
            }
        }
    }
    return msg ;
}

void solve(){
    using Clock = std::chrono::steady_clock;
    auto start_time = Clock::now();

    // cout<<"Please Enter the Cipher Text"<<endl ;
    string original_cipher;  
    getline(cin>>ws,original_cipher);
    
    // Remove ONLY trailing newlines (not all whitespace)
    // This is needed because script input includes \n but terminal input does not
    while (!original_cipher.empty() && 
           (original_cipher.back() == '\n' || original_cipher.back() == '\r')) {
        original_cipher.pop_back();
    }
    
    //this str would be also having the space and not allowed characters
    vector<string>cipher_words;
    string ciphertext = get_cipher(original_cipher,cipher_words);

    //we would be using the trigrams.txt to check the resemeblence and for scoring purpose]
    // cout<<"loading the trigrams and quadgrams dataset"<<endl;
    load_trigrams("trigrams.txt");
    load_quadgrams("quadgrams.txt");
    vector<char>cipher_chars;
    for(auto it:cipher_set){
        cipher_chars.push_back(it);
    }
    // cout<<"\nThe total number of words in the given Cipher Text are : "<<cipher_words.size()<<endl ;
    // cout<<"The number of unique char's in the cipher text are : "<<cipher_chars.size()<<endl ;

    //start with a random key of all lowercase
    string key = "abcdefghijklmnopqrstuvwxyz";
    
    //we would do stimulated annealing (inital randomness and bad decisions allowed , but decreases with time )
    
    //simulated annealing 
    double random_score = 1.0 ;
    int iterations = 0 ;
    int max_iterations = 8500 ;
    string plaintext ;
    int count = 0 ;
    double grades = -10000000.0;
    double limit = 0.9998 ;
    double max_grades = grades ;
    string finalized_key="";

    // cout<<"\nStarting optimization..."<<endl;

    while(count < max_iterations){
        auto now = Clock::now();
        double elapsed =
           std::chrono::duration_cast<std::chrono::duration<double>>(now - start_time).count();

        if(elapsed >= TIME_LIMIT)break ;
        string newkey = randomize_key(key);
        double score_achieved = calculate_score(cipher_chars,newkey,cipher_words);

        if(score_achieved > grades){
            if(score_achieved > max_grades){
                max_grades = score_achieved ;
                finalized_key = newkey ;
            }
            key=newkey;
            grades = score_achieved;
            count=0 ; 

            // if(iterations % 1000 == 0){
            //     // cout<<"Iteration "<<iterations<<" | Best score: "<<max_grades<<endl;
            // }
        }else{
            double random_val = (double)rand()/RAND_MAX ;
            if(random_val < random_score){
                grades = score_achieved ;
                key = newkey ;
            }
            count++ ;
        }

        random_score *= limit ; //we keep on decreasing the proprtion of making poor decisions 
        iterations++ ;
    }

    // cout<<"\nOptimisations are completed . Now Building the plaintext"<<endl;
    plaintext = decrypt(finalized_key,cipher_chars,cipher_words);

    // Remove spaces from plaintext before reconstruction
    int needed = 0;
    for (char c : original_cipher) {
        if (isallowed(c) )
            needed++;
    }
    string plaintext_nospaces = "";
    for(int i=0 ; i<plaintext.size() ; i++){
        if(plaintext[i] != ' ') plaintext_nospaces += plaintext[i];
    }

    if ((int)plaintext_nospaces.size() > needed) {
        plaintext_nospaces.resize(needed);
    }



    // Reconstruct with original formatting
    string final_plaintext;
    int j = 0;

    for (char c : original_cipher) {
        if (c == ' ') {
            final_plaintext += ' ';
        }
        else if (isallowed(c)) {
            if (j < plaintext_nospaces.size())
                final_plaintext += plaintext_nospaces[j++];
        }
        // else: punctuation → ignore
    }
    
    //we would also need to remove punctutations from the cipher text to do the mapping 
    string temp = original_cipher ;
    original_cipher="";
    
    for(int i=0 ; i<temp.size() ; i++){
        if(isallowed(temp[i])){
            original_cipher+=temp[i];
        }else{
            if(isspace(temp[i])){
                original_cipher+=temp[i];
            }
        }
    }


    // Extract the key mapping
    map<char,char>key_mapping;
    for(int i=0 ; i<final_plaintext.size() && i<original_cipher.size() ; i++){
        if (isallowed(final_plaintext[i]) && isallowed(original_cipher[i])) {
            key_mapping[tolower(final_plaintext[i])] = original_cipher[i];
        }
    }
    string final_key = "";
    for(char ch='a' ; ch<='z' ; ch++){
        if(key_mapping.find(ch) != key_mapping.end()){
            final_key += key_mapping[ch];
        }else{
            final_key += 'x';
        }
    }

    // cout<<"\nThe Results are as follow:"<<endl;
    // cout<<"Deciphered Key: "<<final_key<<endl;
    // cout<<"\nThe Decrypted Cipher Text would look like : \n"<<final_plaintext<<endl;

    // ofstream rfile("result.txt");

    // // rfile << "Ciphertext: " << original_cipher << endl << endl;
    // rfile << "Deciphered Plaintext: " << final_plaintext <<endl;
    // rfile << "Deciphered Key: " << final_key << endl;

    // rfile.close();
    cout<<"Deciphered Plaintext: "<<final_plaintext<<endl ;
    cout<<"Deciphered Key: "<<final_key;

    // cout << "\nResults written to result.txt" << endl;
}

signed main(){
    srand(time(NULL));
    solve();
}