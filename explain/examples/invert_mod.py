#!/usr/bin/python

# Invert an integer modulo p.
# Code from http://en.wikibooks.org/wiki/Algorithm_Implementation/Mathematics/Extended_Euclidean_algorithm
# (except test code)

def egcd(a, b):
    x,y, u,v = 0,1, 1,0
    while a != 0:
        q, r = b//a, b%a
        m, n = x-u*q, y-v*q
        b,a, x,y, u,v = a,r, u,v, m,n
    gcd = b
    return gcd, x, y

def modinv(a, m):
    gcd, x, y = egcd(a, m)
    if gcd != 1:
        return None  # modular inverse does not exist
    else:
        return x % m

if __name__ == "__main__":
    inv12 = modinv(12, 7)
    print "Inverse of 12 in mod 7: ", inv12
    twelvetimesinv12 = 12*inv12
    print "Validating:  12 *", inv12, " = ", twelvetimesinv12
    print twelvetimesinv12, " mod 7 = ", twelvetimesinv12%7
