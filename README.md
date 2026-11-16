# Substitution Cipher Cryptanalysis

Breaks monoalphabetic substitution ciphers automatically, with no knowledge of the key, using trigram/quadgram frequency scoring and simulated annealing.

A monoalphabetic substitution cipher has 26! (~4×10²⁶) possible keys, ruling out brute force. This instead treats key recovery as an optimization problem: search over candidate keys, score each decryption by how "English-like" it is (via n-gram log-probabilities), and use simulated annealing to climb toward the true key without getting stuck in local optima.

## Approach

1. Build a log-probability model from trigram/quadgram frequency corpora (`trigrams.txt`, `quadgrams.txt`).
2. Represent the encryption key as a permutation of the alphabet.
3. Generate neighboring keys by swapping two letters at random.
4. Score each candidate decryption as the sum of n-gram log-probabilities.
5. Accept better keys always, worse keys probabilistically (simulated annealing), with acceptance probability decaying over time.
6. Terminate after a fixed number of iterations without improvement, then reconstruct the plaintext with original spacing, punctuation, and casing.

## Build & run

```bash
g++ -O2 -o main main.cpp
./main
```

The program reads the ciphertext from stdin and prints the recovered plaintext and key:

```bash
echo "1981y, \$pp1n1yuux oq@ 2@3s5u1n \$p 1981y, ..." | ./main
```

`trigrams.txt` and `quadgrams.txt` must be present in the working directory — they're the frequency corpora the scorer is built from.

## Example

Given a ciphertext encrypted with an unknown 26-letter substitution key, the solver recovers:

```
Deciphered Plaintext: india, officially the republic of india, is a country in south asia...
Deciphered Key: y5n8@p7q1xwu09v$32460#xxx
```

Letters that never appear in the ciphertext can't be inferred and are left as `x` in the recovered key.

## Limitations

- Performance degrades on very short ciphertexts (not enough n-gram signal to distinguish candidates).
- Relies on the accuracy of the underlying English trigram/quadgram statistics.
- Difficult instances may need multiple runs since simulated annealing is randomized.

## License

Released under the [MIT License](LICENSE).
