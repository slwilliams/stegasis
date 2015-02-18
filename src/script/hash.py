def hash(nums):
    i = 1
    out = 0
    for n in nums:
        out ^= (n%2)*i
        i += 1
    return out

print(hash([1,1,3,7,1,6,7]))
