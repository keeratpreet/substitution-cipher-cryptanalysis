// Breaks a monoalphabetic substitution cipher with no known key, using
// simulated annealing guided by trigram/quadgram log-probability scoring.
// See README.md for the algorithm writeup and readme.pdf for the full report.

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

// Ciphertext letter counts can exceed 32-bit ranges when scores are summed
// over long inputs, so all arithmetic below is done in 64-bit ints.
#define int long long

const double TIME_LIMIT = 110.0 ; // hard wall-clock budget for the annealing search, in seconds

// log-probability tables built from the trigram/quadgram corpora at startup
unordered_map<string,double>trigrams;
unordered_map<string,double>quadgrams;

// fallback log-probability assigned to n-grams that never appear in the corpus,
// so unseen n-grams are penalized rather than crashing the scorer
double net_val_t ;
double net_val_q ;

// distinct ciphertext letters actually present in the input; only these need a
// key mapping, since anything else can't be inferred from this ciphertext
set<char>cipher_set;

// letters/spaces are meaningful for scoring and reconstruction; punctuation is not
bool isallowed(char ch){
    if(ch == ' ' || ch == ',' || ch=='.' || ch==';' ||ch=='!')return false;
    return true ;
}

bool is_space(char ch){
    if(ch==' ')return true ;
    return false ;
}

// Strips punctuation from the ciphertext (keeping letters/spaces), and splits
// it into whitespace-separated words for scoring. Also records every distinct
// ciphertext letter seen, since only those need to appear in the recovered key.
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

// Loads "trigram count" pairs from a corpus file and converts raw counts into
// log10 probabilities (log-space avoids underflow when scores are summed over
// long candidate plaintexts).
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

    for(auto& it:trigrams){
        double prob = (double)it.second/net_count;
        it.second = log10(prob);
    }

    // score assigned to any trigram absent from the corpus: treat it as if it
    // occurred with probability 0.01/net_count, a small but non-zero floor
    net_val_t = log10(0.01/net_count);
}

// Same idea as load_trigrams, for the quadgram corpus.
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

    for(auto& it:quadgrams){
        double prob = (double)it.second/net_count;
        it.second = log10(prob);
    }

    net_val_q = log10(0.01/net_count);
}

// Generates a neighboring key by swapping two randomly chosen positions.
// A swap always preserves the permutation property (every letter still maps
// to exactly one other letter), which is required for a valid substitution key.
string randomize_key(string &str){
    string new_str = str ;
    int x = rand()%str.size();
    int y = rand()%str.size();
    swap(new_str[x],new_str[y]);
    return new_str ;
}

// Scores how "English-like" a single decrypted word is, as the sum of its
// trigram and quadgram log-probabilities. Higher (less negative) is better.
double fit_score(string word){
    double score = 0 ;
    int n = word.size();

    // trigram contribution: every overlapping 3-letter window
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

    // quadgram contribution: every overlapping 4-letter window
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

// Decrypts the ciphertext under a candidate key and sums fit_score() over
// every word, giving the total fitness of that key. This is the objective
// function simulated annealing is trying to maximize.
double calculate_score(vector<char>&cipher_chars , string &key ,vector<string>&cipher_words){
    // build the ciphertext-letter -> plaintext-letter mapping implied by this key:
    // cipher_chars[i] decrypts to key[i]
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

// Applies a key to the full ciphertext word list and returns the decrypted
// text (words separated by single spaces; original punctuation is restored
// later by solve()).
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

// Main driver: reads ciphertext from stdin, runs simulated annealing over the
// key space, and prints the recovered plaintext and key.
void solve(){
    using Clock = std::chrono::steady_clock;
    auto start_time = Clock::now();

    string original_cipher;
    getline(cin>>ws,original_cipher);

    // Remove ONLY trailing newlines (not all whitespace) — needed because
    // piped/script input includes a trailing \n that interactive input does not.
    while (!original_cipher.empty() &&
           (original_cipher.back() == '\n' || original_cipher.back() == '\r')) {
        original_cipher.pop_back();
    }

    vector<string>cipher_words;
    string ciphertext = get_cipher(original_cipher,cipher_words);

    load_trigrams("trigrams.txt");
    load_quadgrams("quadgrams.txt");
    vector<char>cipher_chars;
    for(auto it:cipher_set){
        cipher_chars.push_back(it);
    }

    // start from the identity key (a->a, b->b, ...); simulated annealing will
    // move away from this via random swaps
    string key = "abcdefghijklmnopqrstuvwxyz";

    // Simulated annealing: always accept a strictly better key; otherwise
    // accept a worse one with probability `random_score`, which decays every
    // iteration (multiplied by `limit`). This lets the search escape local
    // optima early on while converging to a local hill-climb later.
    double random_score = 1.0 ;
    int iterations = 0 ;
    int max_iterations = 8500 ;   // stop after this many iterations without improvement
    string plaintext ;
    int count = 0 ;               // iterations since the last accepted-as-best key
    double grades = -10000000.0;  // current key's score
    double limit = 0.9998 ;       // per-iteration decay factor for random_score
    double max_grades = grades ;  // best score seen so far across the whole run
    string finalized_key="";

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
        }else{
            // accept a worse key anyway with probability random_score (annealing)
            double random_val = (double)rand()/RAND_MAX ;
            if(random_val < random_score){
                grades = score_achieved ;
                key = newkey ;
            }
            count++ ;
        }

        random_score *= limit ; // cool down: poor decisions become progressively less likely
        iterations++ ;
    }

    plaintext = decrypt(finalized_key,cipher_chars,cipher_words);

    // decrypt() joins words with single spaces, which may not match the
    // original ciphertext's spacing exactly — so strip spaces here and
    // re-insert them from original_cipher's actual layout below.
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

    // Reconstruct the plaintext with the ORIGINAL ciphertext's spacing:
    // walk original_cipher char by char, copying spaces through as-is and
    // consuming one decrypted letter per allowed (letter) position. Punctuation
    // is dropped, matching how it was excluded from scoring.
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

    // Rebuild original_cipher itself as letters+spaces only (punctuation
    // stripped), so it lines up 1:1 with final_plaintext for key extraction below.
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

    // Extract the final plaintext->cipher letter mapping directly by walking
    // the two aligned strings position by position, rather than reusing the
    // annealed key (which is only valid over cipher_chars' original ordering).
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
            // this plaintext letter never appeared in the ciphertext, so its
            // mapping can't be inferred — mark it with a placeholder
            final_key += 'x';
        }
    }

    cout<<"Deciphered Plaintext: "<<final_plaintext<<endl ;
    cout<<"Deciphered Key: "<<final_key;
}

signed main(){
    srand(time(NULL));
    solve();
}
