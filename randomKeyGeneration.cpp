#include <Arduino.h>

void setup() {
	init();
	Serial.begin(9600);
	pinMode(A1, INPUT);
}

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

int main() {
	setup();
	while(true) {
		unsigned int smallprime = primerange(14);
		unsigned int biggerprime = primerange(15);
		uint32_t toti = totient(smallprime,biggerprime);
		uint32_t e = publickey(toti);
		uint32_t mod = modulus(smallprime, biggerprime);
		int32_t euci = ext_euclid(e, toti);
		int32_t rightmod = reduce_mod(euci, mod);
		Serial.println(e);
		Serial.println(toti);
		Serial.println(euci);
		Serial.println(mod);
		Serial.println(rightmod);
		Serial.println();
		delay(1000);
	}

	Serial.flush();
	return 0;
}



