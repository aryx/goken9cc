// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package rsa

import (
	"big"
	"bytes"
	"crypto/rand"
	"crypto/sha1"
	"encoding/base64"
	"encoding/hex"
	"io"
	"testing"
	"testing/quick"
)

func decodeBase64(in string) []byte {
	out := make([]byte, base64.StdEncoding.DecodedLen(len(in)))
	n, err := base64.StdEncoding.Decode(out, []byte(in))
	if err != nil {
		return nil
	}
	return out[0:n]
}

type DecryptPKCS1v15Test struct {
	in, out string
}

// These test vectors were generated with `openssl rsautl -pkcs -encrypt`
var decryptPKCS1v15Tests = []DecryptPKCS1v15Test{
	DecryptPKCS1v15Test{
		"gIcUIoVkD6ATMBk/u/nlCZCCWRKdkfjCgFdo35VpRXLduiKXhNz1XupLLzTXAybEq15juc+EgY5o0DHv/nt3yg==",
		"x",
	},
	DecryptPKCS1v15Test{
		"Y7TOCSqofGhkRb+jaVRLzK8xw2cSo1IVES19utzv6hwvx+M8kFsoWQm5DzBeJCZTCVDPkTpavUuEbgp8hnUGDw==",
		"testing.",
	},
	DecryptPKCS1v15Test{
		"arReP9DJtEVyV2Dg3dDp4c/PSk1O6lxkoJ8HcFupoRorBZG+7+1fDAwT1olNddFnQMjmkb8vxwmNMoTAT/BFjQ==",
		"testing.\n",
	},
	DecryptPKCS1v15Test{
		"WtaBXIoGC54+vH0NH0CHHE+dRDOsMc/6BrfFu2lEqcKL9+uDuWaf+Xj9mrbQCjjZcpQuX733zyok/jsnqe/Ftw==",
		"01234567890123456789012345678901234567890123456789012",
	},
}

func TestDecryptPKCS1v15(t *testing.T) {
	for i, test := range decryptPKCS1v15Tests {
		out, err := DecryptPKCS1v15(nil, rsaPrivateKey, decodeBase64(test.in))
		if err != nil {
			t.Errorf("#%d error decrypting", i)
		}
		want := []byte(test.out)
		if bytes.Compare(out, want) != 0 {
			t.Errorf("#%d got:%#v want:%#v", i, out, want)
		}
	}
}

func TestEncryptPKCS1v15(t *testing.T) {
	random := rand.Reader
	k := (rsaPrivateKey.N.BitLen() + 7) / 8

	tryEncryptDecrypt := func(in []byte, blind bool) bool {
		if len(in) > k-11 {
			in = in[0 : k-11]
		}

		ciphertext, err := EncryptPKCS1v15(random, &rsaPrivateKey.PublicKey, in)
		if err != nil {
			t.Errorf("error encrypting: %s", err)
			return false
		}

		var rand io.Reader
		if !blind {
			rand = nil
		} else {
			rand = random
		}
		plaintext, err := DecryptPKCS1v15(rand, rsaPrivateKey, ciphertext)
		if err != nil {
			t.Errorf("error decrypting: %s", err)
			return false
		}

		if bytes.Compare(plaintext, in) != 0 {
			t.Errorf("output mismatch: %#v %#v", plaintext, in)
			return false
		}
		return true
	}

	quick.Check(tryEncryptDecrypt, nil)
}

// These test vectors were generated with `openssl rsautl -pkcs -encrypt`
var decryptPKCS1v15SessionKeyTests = []DecryptPKCS1v15Test{
	DecryptPKCS1v15Test{
		"e6ukkae6Gykq0fKzYwULpZehX+UPXYzMoB5mHQUDEiclRbOTqas4Y0E6nwns1BBpdvEJcilhl5zsox/6DtGsYg==",
		"1234",
	},
	DecryptPKCS1v15Test{
		"Dtis4uk/q/LQGGqGk97P59K03hkCIVFMEFZRgVWOAAhxgYpCRG0MX2adptt92l67IqMki6iVQyyt0TtX3IdtEw==",
		"FAIL",
	},
	DecryptPKCS1v15Test{
		"LIyFyCYCptPxrvTxpol8F3M7ZivlMsf53zs0vHRAv+rDIh2YsHS69ePMoPMe3TkOMZ3NupiL3takPxIs1sK+dw==",
		"abcd",
	},
	DecryptPKCS1v15Test{
		"bafnobel46bKy76JzqU/RIVOH0uAYvzUtauKmIidKgM0sMlvobYVAVQPeUQ/oTGjbIZ1v/6Gyi5AO4DtHruGdw==",
		"FAIL",
	},
}

func TestEncryptPKCS1v15SessionKey(t *testing.T) {
	for i, test := range decryptPKCS1v15SessionKeyTests {
		key := []byte("FAIL")
		err := DecryptPKCS1v15SessionKey(nil, rsaPrivateKey, decodeBase64(test.in), key)
		if err != nil {
			t.Errorf("#%d error decrypting", i)
		}
		want := []byte(test.out)
		if bytes.Compare(key, want) != 0 {
			t.Errorf("#%d got:%#v want:%#v", i, key, want)
		}
	}
}

func TestNonZeroRandomBytes(t *testing.T) {
	random := rand.Reader

	b := make([]byte, 512)
	err := nonZeroRandomBytes(b, random)
	if err != nil {
		t.Errorf("returned error: %s", err)
	}
	for _, b := range b {
		if b == 0 {
			t.Errorf("Zero octet found")
			return
		}
	}
}

type signPKCS1v15Test struct {
	in, out string
}

// These vectors have been tested with
//   `openssl rsautl -verify -inkey pk -in signature | hexdump -C`
var signPKCS1v15Tests = []signPKCS1v15Test{
	signPKCS1v15Test{"Test.\n", "a4f3fa6ea93bcdd0c57be020c1193ecbfd6f200a3d95c409769b029578fa0e336ad9a347600e40d3ae823b8c7e6bad88cc07c1d54c3a1523cbbb6d58efc362ae"},
}

func TestSignPKCS1v15(t *testing.T) {
	for i, test := range signPKCS1v15Tests {
		h := sha1.New()
		h.Write([]byte(test.in))
		digest := h.Sum()

		s, err := SignPKCS1v15(nil, rsaPrivateKey, HashSHA1, digest)
		if err != nil {
			t.Errorf("#%d %s", i, err)
		}

		expected, _ := hex.DecodeString(test.out)
		if bytes.Compare(s, expected) != 0 {
			t.Errorf("#%d got: %x want: %x", i, s, expected)
		}
	}
}

func TestVerifyPKCS1v15(t *testing.T) {
	for i, test := range signPKCS1v15Tests {
		h := sha1.New()
		h.Write([]byte(test.in))
		digest := h.Sum()

		sig, _ := hex.DecodeString(test.out)

		err := VerifyPKCS1v15(&rsaPrivateKey.PublicKey, HashSHA1, digest, sig)
		if err != nil {
			t.Errorf("#%d %s", i, err)
		}
	}
}

func bigFromString(s string) *big.Int {
	ret := new(big.Int)
	ret.SetString(s, 10)
	return ret
}

// In order to generate new test vectors you'll need the PEM form of this key:
// -----BEGIN RSA PRIVATE KEY-----
// MIIBOgIBAAJBALKZD0nEffqM1ACuak0bijtqE2QrI/KLADv7l3kK3ppMyCuLKoF0
// fd7Ai2KW5ToIwzFofvJcS/STa6HA5gQenRUCAwEAAQJBAIq9amn00aS0h/CrjXqu
// /ThglAXJmZhOMPVn4eiu7/ROixi9sex436MaVeMqSNf7Ex9a8fRNfWss7Sqd9eWu
// RTUCIQDasvGASLqmjeffBNLTXV2A5g4t+kLVCpsEIZAycV5GswIhANEPLmax0ME/
// EO+ZJ79TJKN5yiGBRsv5yvx5UiHxajEXAiAhAol5N4EUyq6I9w1rYdhPMGpLfk7A
// IU2snfRJ6Nq2CQIgFrPsWRCkV+gOYcajD17rEqmuLrdIRexpg8N1DOSXoJ8CIGlS
// tAboUGBxTDq3ZroNism3DaMIbKPyYrAqhKov1h5V
// -----END RSA PRIVATE KEY-----

var rsaPrivateKey = &PrivateKey{
	PublicKey: PublicKey{
		N: bigFromString("9353930466774385905609975137998169297361893554149986716853295022578535724979677252958524466350471210367835187480748268864277464700638583474144061408845077"),
		E: 65537,
	},
	D: bigFromString("7266398431328116344057699379749222532279343923819063639497049039389899328538543087657733766554155839834519529439851673014800261285757759040931985506583861"),
	P: bigFromString("98920366548084643601728869055592650835572950932266967461790948584315647051443"),
	Q: bigFromString("94560208308847015747498523884063394671606671904944666360068158221458669711639"),
}
