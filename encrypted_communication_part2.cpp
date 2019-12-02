/*
    Encrypted Arduino Communication Part 2
    CMPUT 274 Fall 2019
    Celine Fong (1580124)
    Claire Martin 
*/

#include <Arduino.h>

/*
    Generates a random k-bit number up to 2^32-1

    Arguments:
        k (unsigned int): The number of bits the random number will use

    Returns:
        rand (unsigned integer): A random k-bit number 
*/
unsigned int randomGenerator(unsigned int k) {
    unsigned int rand = 0;
    for (unsigned int i = 0; i < k; i++) {
        // take least significant bit of value read from analogRead
        int LSB = analogRead(A1) & 1;
        // append the LSB that was just read from A1 to the end of the random binary number
        rand = (LSB << i | rand);
        delay(5);
    }
    return rand;
}

/*
    Takes the upper square root of n
    IMPORTANT: This funcion was taken from the primality morning problem presented in class

    Arguments:
        n (unsigned int): The integer whose upper square root is desired

    Returns:
        d (unsigned int): The smallest integer such that d*d will be greater than n 
*/
unsigned int upper_sqrt(unsigned int n) {
    // returns smallest integer d such that d*d will be greater than n
    unsigned int d = sqrt((double) n);
    while (d*d <= n && d <= (1<<16)) {
        ++d;
    }

    return d;
}

/*
    Determines if a number is prime or not
    IMPORTANT: Taken from Celine Fong (1580124) solution to primality morning problem presented in class

    Arguments:
        n (unsigned int): The number that will be tested for primality

    Returns:
        is_prime (bool): true if n is prime, false if not
*/
bool primality(unsigned int n) {
    unsigned int i = 2;
    // initially assume the number is prime
    bool is_prime = true;
    // check all integers between 2 and the square root of n
    while ((i < upper_sqrt(n)) && (is_prime == true)) {
        // if n is divisible by any of these numbers, it is not prime
        if (n % i == 0) {
            // break out of loop
            is_prime = false;
        }
        i++;
    }
    return is_prime;    
}

/*
    Generates a random prime number in a given range.

    Arguments:
        k (unsigned int): Indicates range to generate prime number within. (2^k to 2^(k+1))

    Returns:
        randNum (unsigned int): Random prime number in range 2^k to 2^(k+1)
*/
unsigned int primerange(unsigned int k) {
    // first generate a random k-bit number and add it to 2^k
    unsigned int randNum = randomGenerator(k) + (1 << k);
    while (!primality(randNum)) {
        // increment random number if it isn't prime
        randNum++;
        // wrap around if number goes over the range
        if (randNum >= (1 << (k+1))) {
            randNum = (randNum % (1<<(k+1))) + (1<<k);
        }
    }
    return randNum;
}

/*
    Generates the required modulus for RSA encryption

    Arguments:
        p (uint32_t): Random prime number between 2^14 to 2^15
        q (uint32_t): Random prime number between 2^15 to 2^16

    Returns:
        n (uint32_t): modulus for RSA encryption
*/
uint32_t modulus(uint32_t p, uint32_t q) {
    uint32_t n = p*q;
    return n;
}

/*
    Calculates totient(phi)

    Arguments:
        p (uint32_t): Random prime number between 2^14 to 2^15
        q (uint32_t): Random prime number between 2^15 to 2^16

    Returns:
        tot (uint32_t): totient of p and q
*/
uint32_t totient(uint32_t p, uint32_t q) {
    uint32_t tot = (p-1)*(q-1);
    return tot;
}

/*
    Determine the greatest common divior of two integers.
    IMPORTANT: This function was taken from GCD program posted to eclass.

    Arguments:
        a (uint32_t): Integer used in conjunction with b to calculate greatest common divisor
        b (uint32_t): Integer used in conjunction with a to calculate greatest common divisor

    Returns:
        a (uint32_t): Greatest common divisor of a and b
*/
uint32_t gcd_euclid_fast(uint32_t a, uint32_t b) {
    while (b > 0) {
    a %= b;

    // now swap them
    uint32_t tmp = a;
    a = b;
    b = tmp;
    }
    return a; // b is 0
}

/*
    Generates the public key for RSA encryption

    Arguments:
        tot (uint32_t): the totient of two randomly generated prime numbers

    Returns:
        pubKey (uint32_t): the public key
*/
uint32_t publickey(uint32_t tot) {
    // first, generate a random 15-bit number
    uint32_t pubKey = randomGenerator(15);
    // ensure that the public key satisfies the condition gcd(pubKey, tot) == 1
    while (gcd_euclid_fast(pubKey, tot) != 1) {
        // if it doesn't satisfy the condition, keep incrementing until find a number that does
        pubKey++;
        // ensures that the key never exceeds 15 bits
        if (pubKey >= (1<<15)) {
            pubKey = 1 + (pubKey % (1<<15));
        } 
        // ensures the key never exceeds totient
        else if (pubKey >= tot) {
            pubKey = 1 + pubKey % tot;
        }
    }
    return pubKey;
}

/*
    Finds the modular inverse of the public key (e).
    IMPORTANT: Adapted from template code given in the Extended Euclidean Algorithm worksheet on eclass

    Arguments:
        e (uint32_t): The public key of the given Arduino
        phi (uint32_t): The totient of two randomly generated prime numbers

    Returns:
        mod_inv (int32_t): The modular inverse of given public key
*/
int32_t ext_euclid(uint32_t e, uint32_t phi) {
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
    int32_t mod_inv = s[i-1];
    return mod_inv;
}

/*
    Determines the modular equivalent of an integer with a given modulus

    Arguments:
        x (int32_t): Integer the modular equivalent is desired from
        m (uint32_t): Modulus the integer will be reduced with

    Returns:
        mod (int32_t): The reduced integer
*/
int32_t reduce_mod(int32_t x, uint32_t m) {
    // if the integer is positive or zero, simply return the x mod m
    if (x>=0) {
        int32_t mod = (x%m);
        return mod;
    } 
    // if the integer is negative, find the modular equivalent and return that positive integer
    else if (x < 0) {
        uint32_t z = (-x/m) + 1;
        int32_t mod = ((x+z*m) % m);
        return mod;
    }
}

// declare global variables for server/client keys and moduli
uint32_t serverPublicKey;
uint32_t serverPrivateKey;
uint32_t serverModulus;

uint32_t clientPublicKey;
uint32_t clientPrivateKey;
uint32_t clientModulus;

/*
    Determines the modular equivalent of an integer with a given modulus

    Arguments:
        serverPublicKey (uint32_t&): Pass-by reference to server's public key
        serverPrivateKey (uint32_t&): Pass-by reference to server's private key
        serverModulus (uint32_t&): Pass-by reference to client's modulus

    Returns:
        Nothing, simply updates pass-by values
*/
void serverKeyGeneration(uint32_t& serverPublicKey, uint32_t& serverPrivateKey, uint32_t& serverModulus) {
    // generate random prime number between 2^14 and 2^15
    unsigned int smallprime = primerange(14);
    // generate random prime number between 2^15 and 2^16
    unsigned int biggerprime = primerange(15);
    // calculate totient of the two primes
    uint32_t toti = totient(smallprime,biggerprime);
    // generates the server's public key
    serverPublicKey = publickey(toti);
    // generates server modulus given the two primes
    serverModulus = modulus(smallprime, biggerprime);
    // generates the modular inverse of the serverPublicKey
    int32_t euci = ext_euclid(serverPublicKey, toti);
    // generates the server's private key by adjusting euci to ensure a positive integer
    serverPrivateKey = reduce_mod(euci, serverModulus);
    Serial.println("generated keys for server");
}

// same as above, but for the client
void clientKeyGeneration(uint32_t& clientPublicKey, uint32_t& clientPrivateKey, uint32_t& clientModulus) {
    unsigned int smallprime = primerange(14);
    unsigned int biggerprime = primerange(15);
    uint32_t toti = totient(smallprime,biggerprime);
    clientPublicKey = publickey(toti);
    clientModulus = modulus(smallprime, biggerprime);
    int32_t euci = ext_euclid(clientPublicKey, toti);
    clientPrivateKey = reduce_mod(euci, clientModulus);
    Serial.println("generated keys for client");
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

// All code below until otherwise indicated was taken from the Major Assignment 2 Part 1 Solution posted to eclass
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

// end of Major Assignemnt 2 Part 1 Solution provided on eclass 

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
        while (current != DataExchange) {
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
            while (current == WaitForKey) {
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
            while (current == WaitForAck) {
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
    Serial.println("hate you");
    uint32_t d, n, e, m;
    uint32_t keyArray[2];
    uint32_t Key, Mod;

    // Determine our role and the encryption keys.
    if (isServer()) {
        Serial.println("Server");
        // generate keys for server
        serverKeyGeneration(serverPublicKey, serverPrivateKey, serverModulus);
        d = serverPrivateKey;
        n = serverModulus;
        // e = clientPublicKey;
        // m = clientModulus;
        Key = serverPublicKey;
        Mod = n;
    } else {
        Serial.println("Client");
        // generate keys for client
        clientKeyGeneration(clientPublicKey, clientPrivateKey, clientModulus);
        d = clientPrivateKey;
        n = clientModulus;
        // e = serverPublicKey;
        // m = serverModulus;
        Key = clientPublicKey;
        Mod = n;
    }
    // Perform Handshake
    handshake(Key, Mod, keyArray);
    e = keyArray[0];
    m = keyArray[1];
    // Now enter the communication phase.
    communication(d, n, e, m);

    // Should never get this far (communication has an infite loop).
    return 0;
}
