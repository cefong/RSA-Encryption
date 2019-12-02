/*
    Encrypted Arduino Communication Part 1 Solution
    CMPUT 274 Fall 2019
    Written by Ian DeHaan
    Tweaks by Zac Friggstad
*/
#include <Arduino.h>
using namespace std;

unsigned int randomGenerator(unsigned int k) {
    // generates a random k-bit number
    // can only generate random numbers up to (2^32)-1
    unsigned int rand = 0;
    for (unsigned int i = 0; i < k; i++) {
        int LSB = analogRead(A1) & 1;
        // append the LSB that was just read from A1 to the end of the random binary number
        rand = (LSB << i | rand);
        delay(5);
    }
    return rand;
}

unsigned int upper_sqrt(unsigned int n) {
    // returns smallest integer d such that d*d will be greater than n
    unsigned int d = sqrt((double) n);
    while (d*d <= n && d <= (1<<16)) {
        ++d;
    }

    return d;
}

bool primality(unsigned int n) {
    // returns true if n is prime, false if not
    unsigned int i = 2;
    bool is_prime = true;
    while ((i < upper_sqrt(n)) && (is_prime == true)) {
        if (n % i == 0) {
            is_prime = false;
        }
        i++;
    }
    return is_prime;    
}

unsigned int primerange(int k) {
    // generates a prime number in range 2^k to 2^(k+1)
    // first generate a random k-bit number and add it to 2^k
    unsigned int randNum = randomGenerator(k) + (1 << k);
    while (!primality(randNum)) {
        randNum++;
        // wrap around if number goes over the range
        if (randNum >= (1 << (k+1))) {
            randNum = (randNum % (1<<(k+1))) + (1<<k);
        }
    }
    return randNum;
}

uint32_t modulus(uint32_t p, uint32_t q) {
    // generates the modulus given prime numbers p and q
    uint32_t n = p*q;
    return n;
}

uint32_t totient(uint32_t p, uint32_t q) {
    // calculates the totient(phi) given prime number p and q
    uint32_t tot = (p-1)*(q-1);
    return tot;
}

uint32_t gcd_euclid_fast(uint32_t a, uint32_t b) {
    // returns the greatest common divisor of a and b
    while (b > 0) {
    a %= b;

    // now swap them
    uint32_t tmp = a;
    a = b;
    b = tmp;
    }
    return a; // b is 0
}

uint32_t publickey(uint32_t tot) {
    // returns the public key given the totient
    uint32_t pubKey = randomGenerator(15);
    while (gcd_euclid_fast(pubKey, tot) != 1) {
        pubKey++;
        if (pubKey >= (1<<15)) {
            pubKey = (pubKey % (1<<15)) + (1<<14);
        }
    }
    return pubKey;
}

int32_t ext_euclid(uint32_t e, uint32_t phi) {
    // returns the modular inverse of the public key, e
    uint32_t r[40];
    int32_t s[40];
    r[0] = e; r[1] = phi;
    s[0] = 1; s[1] = 0;
    int i = 1;
    while (r[i] > 0) {
        int q = r[i-1]/r[i];
        r[i+1] = r[i-1] - q*r[i];
        s[i+1] = s[i-1] - q*s[i];
        i++;
    }
    return s[i-1];
}

int32_t reduce_mod(int32_t x, uint32_t m) {
    // returns the private key given the modulus and the modular inverse of e
    if (x>=0) {
        return (x%m);
    } else if (x < 0) {
        uint32_t z = (-x/m) + 1;
        return ((x+z*m) % m);
    }
}


// Constants from the assn. spec
// Primes: 307, 311
const uint32_t serverPublicKey;
const uint32_t serverPrivateKey;
const uint32_t serverModulus;

// Generator: 271, 313
const uint32_t clientPublicKey;
const uint32_t clientPrivateKey;
const uint32_t clientModulus;

int32_t clientKeyGeneration(uint32_t& serverPublicKey, uint32_t& serverPrivateKey, uint32_t& serverModulus) {
    unsigned int smallprime = primerange(14);
    unsigned int biggerprime = primerange(15);
    uint32_t toti = totient(smallprime,biggerprime);
    serverPublicKey = publickey(toti);
    serverModulus = modulus(smallprime, biggerprime);
    int32_t euci = ext_euclid(serverPublicKey, toti);
    serverPrivateKey = reduce_mod(euci, serverModulus);
}

int32_t serverKeyGeneration(uint32_t& clientPublicKey, uint32_t& clientPrivateKey, uint32_t& clientModulus) {
    unsigned int smallprime = primerange(14);
    unsigned int biggerprime = primerange(15);
    uint32_t toti = totient(smallprime,biggerprime);
    clientPublicKey = publickey(toti);
    clientModulus = modulus(smallprime, biggerprime);
    int32_t euci = ext_euclid(clientPublicKey, toti);
    clientPrivateKey = reduce_mod(euci, clientModulus);
}

const int serverPin = 13;

enum StateNames {
    WaitForAck, DataExchange, Listen, WaitForKey
};

/*
    Returns true if arduino is server, false if arduino is client
*/
bool isServer() {
    if (digitalRead(serverPin) == HIGH) {
        return true;
    } else {
        return false;
    }
}

/*
    Compute and return (a*b)%m
    Note: m must be less than 2^31
    Arguments:
        a (uint32_t): The first multiplicant
        b (uint32_t): The second multiplicant
        m (uint32_t): The mod value
    Returns:
        result (uint32_t): (a*b)%m
*/
uint32_t multMod(uint32_t a, uint32_t b, uint32_t m) {
    uint32_t result = 0;
    uint32_t dblVal = a%m;
    uint32_t newB = b;

    // This is the result of working through the worksheet.
    // Notice the extreme similarity with powmod.
    while (newB > 0) {
        if (newB & 1) {
            result = (result + dblVal) % m;
        }
        dblVal = (dblVal << 1) % m;
        newB = (newB >> 1);
    }

    return result;
}


/*
    NOTE: This was modified using our multMod function, but is otherwise the
    function powModFast provided in the lectures.

    Compute and return (a to the power of b) mod m.
      Example: powMod(2, 5, 13) should return 6.
*/
uint32_t powMod(uint32_t a, uint32_t b, uint32_t m) {
    uint32_t result = 1 % m;
    uint32_t sqrVal = a % m;  // stores a^{2^i} values, initially 2^{2^0}
    uint32_t newB = b;

    // See the lecture notes for a description of why this works.
    while (newB > 0) {
        if (newB & 1) {  // evalutates to true iff i'th bit of b is 1 in the i'th iteration
            result = multMod(result, sqrVal, m);
        }
        sqrVal = multMod(sqrVal, sqrVal, m);
        newB = (newB >> 1);
    }

    return result;
}



/* Waits for a certain number of bytes on Serial3 or timeout.
    Arguments:
        nbytes: number of bytes needed to read
        timeout: time period (ms), negative number will turn off timeouts

    Return:
        true if required number of bytes have arrived
*/
bool wait_on_serial3(uint8_t nbytes, long timeout) {
    unsigned long deadline = millis() + timeout;
    while (Serial3.available() < nbytes && (timeout < 0 || millis() < deadline)) {
        delay(1);
    }
    return (Serial3.available() >= nbytes);
}



/** Writes an uint32_t to Serial3, starting from the least-significant
 * and finishing with the most significant byte.
 */
void uint32_to_serial3(uint32_t num) {
    Serial3.write((char) (num >> 0));
    Serial3.write((char) (num >> 8));
    Serial3.write((char) (num >> 16));
    Serial3.write((char) (num >> 24));
}


/** Reads an uint32_t from Serial3, starting from the least-significant
 * and finishing with the most significant byte.
 */
uint32_t uint32_from_serial3() {
    uint32_t num = 0;
    num = num | ((uint32_t) Serial3.read()) << 0;
    num = num | ((uint32_t) Serial3.read()) << 8;
    num = num | ((uint32_t) Serial3.read()) << 16;
    num = num | ((uint32_t) Serial3.read()) << 24;
    return num;
}


/*
    Encrypts using RSA encryption.

    Arguments:
        c (char): The character to be encrypted
        e (uint32_t): The partner's public key
        m (uint32_t): The partner's modulus

    Return:
        The encrypted character (uint32_t)
*/
uint32_t encrypt(char c, uint32_t e, uint32_t m) {
    return powMod(c, e, m);
}


/*
    Decrypts using RSA encryption.

    Arguments:
        x (uint32_t): The communicated integer
        d (uint32_t): The Arduino's private key
        n (uint32_t): The Arduino's modulus

    Returns:
        The decrypted character (char)
*/
char decrypt(uint32_t x, uint32_t d, uint32_t n) {
    return (char) powMod(x, d, n);
}


void handshake(uint32_t d, uint32_t n, uint32_t arr[]) {
    // d, n are the keys to be sent
    uint32_t e, m;
    bool firstTime;
    StateNames current;
    // e, m are the keys to be received
    // WaitForAck, DataExchange, Listen, WaitForKey
    
    if (isServer()) {
        // if server
        // d, n are server keys
        // e, m are client keys
        current = Listen;
        // if we have reached Data Exchane stage, stop handshake
        while (current == Listen) {
            // wait to receive connection request 'C' on Serial3
            // reset firstTime variable so that the Server will send its keys
            Serial.println("Listening");
            firstTime = true;
            if (wait_on_serial3(1, 1000)) {
                // keeps waiting to read 'C'
                char readC = Serial3.read();
                if (readC == 'C') {
                    // if the character read was 'C', continue to next state
                    // otherwise, will keep listening
                    current = WaitForKey;
                }
            }
        }
        if (current == WaitForKey) {
            // once C received, read client public key (exp and mod)
            // wait for 1s to read the 8 bytes of the keys
            Serial.println("Waiting for Key");
            if (wait_on_serial3(8, 1000)) {
                // if it could read 8 bytes within 1s
                // first read e, 4 bytes
                e = uint32_from_serial3();
                // next read m, 4 bytes
                m = uint32_from_serial3();
                Serial.println("Received keys");
                if (firstTime) {
                    // if this is the first time you've hit WaitForKey state
                    // prevents the arduino from sending its keys twice
                    // send 'A', then send server public keys
                    Serial.println("Sending keys");
                    Serial3.write('A');
                    uint32_to_serial3(d);
                    uint32_to_serial3(n);
                }
                // move to next state
                current = WaitForAck;
            } else {
                // this will only happen if it couldn't read 8 bytes or took too long
                // go back to listening state and start again waiting for C
                current = Listen;
            }
        }
        if (current == WaitForAck) {
            // wait for 'A' from client on Serial3
            Serial.println("Waiting for Ack");
            if (wait_on_serial3(1, 1000)) {
                // if it could read a character within 1s
                char readA = Serial3.read();
                if (readA == 'A') {
                    // if character was 'A', move to data exchange
                    Serial.println("Received- Data Exchange Ready");
                    current = DataExchange;
                } else if (readA == 'C') {
                    // if receives 'C' instead of 'A', read client keys but do not send server keys
                    // if character was 'C', move back to waiting for key state
                    // firstTime is reset to false so Server will not send its keys again
                    firstTime = false;
                    current = WaitForKey;
                } else {
                    // if the byte was something other than 'A' or 'C', reset to Listen
                    current = Listen;
                }
            } else {
                // if it couldn't read a byte or if it took too long
                current = Listen;
            }   
        }
    }

    if (!isServer()) {
        // if client
        Serial.println("I am the client");
        // d, n are client keys
        // e, m are server keys
        current = WaitForAck;
        while (current == WaitForAck) {
            Serial.println("Waiting for Ack");
            // if you haven't already received 'A'
            // keep sending 'C' and client keys
            Serial3.write('C');
            uint32_to_serial3(d);
            uint32_to_serial3(n);
            // if no 'A' is read from server, loop will repeat and send 'C' and keys again
            if (wait_on_serial3(1, 1000)) {
                // if character available within 1s
                char readA = Serial3.read();
                if (readA == 'A' && wait_on_serial3(8, 1000)) {
                    // if character read was A and client could read 8 bytes in time
                    // read server keys
                    Serial.println("Received- reading keys");
                    e = uint32_from_serial3();
                    m = uint32_from_serial3();
                    // send 'A' back
                    Serial3.write('A');
                    // state is now Data Exchange
                    current = DataExchange;
                }
            }
        }
    }
    if (current == DataExchange) {
        arr[0] = e;
        arr[1] = m;
    }
    return 0;
}


/*
    Core communication loop
    d, n, e, and m are according to the assignment spec
*/
void communication(uint32_t d, uint32_t n, uint32_t e, uint32_t m) {
    // Consume all early content from Serial3 to prevent garbage communication
    while (Serial3.available()) {
        Serial3.read();
    }

    // Enter the communication loop
    while (true) {
        // Check if the other Arduino sent an encrypted message.
        if (Serial3.available() >= 4) {
            // Read in the next character, decrypt it, and display it
            uint32_t read = uint32_from_serial3();
            Serial.print(decrypt(read, d, n));
        }

        // Check if the user entered a character.
        if (Serial.available() >= 1) {
            char byteRead = Serial.read();
            // Read the character that was typed, echo it to the serial monitor,
            // and then encrypt and transmit it.
            if ((int) byteRead == '\r') {
                // If the user pressed enter, we send both '\r' and '\n'
                Serial.print('\r');
                uint32_to_serial3(encrypt('\r', e, m));
                Serial.print('\n');
                uint32_to_serial3(encrypt('\n', e, m));
            } else {
                Serial.print(byteRead);
                uint32_to_serial3(encrypt(byteRead, e, m));
            }
        }
    }
}


/*
    Performs basic Arduino setup tasks.
*/
void setup() {
    init();
    Serial.begin(9600);
    Serial3.begin(9600);

    Serial.println("Welcome to Arduino Chat!");
}


/*
    The entry point to our program.
*/
int main() {
    setup();
    uint32_t d, n, e, m;
    uint32_t keyArray[2];

    // Determine our role and the encryption keys.
    if (isServer()) {
        Serial.println("Server");
        d = serverPrivateKey;
        n = serverModulus;
        // e = clientPublicKey;
        // m = clientModulus;
        uint32_t sKey = serverPublicKey;
        uint32_t sMod = n;
        handshake(sKey, sMod, keyArray);
        e = keyArray[0];
        m = keyArray[1];
    } else {
        Serial.println("Client");
        d = clientPrivateKey;
        n = clientModulus;
        // e = serverPublicKey;
        // m = serverModulus;
        uint32_t cKey = clientPublicKey;
        uint32_t cMod = n;
        handshake(cKey, cMod, keyArray);
        e = keyArray[0];
        m = keyArray[1];
    }
    // Now enter the communication phase.
    communication(d, n, e, m);

    // Should never get this far (communication has an infite loop).

    return 0;
}
