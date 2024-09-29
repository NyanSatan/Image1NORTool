# Image1NORTool

Tiny tool you can use to fix or create **Image1** header of **LLB** in a **S5L8900X** device's NOR

**Image1** format used to store **LLB** in NOR differs a lot from **Image1**s you can find in an IPSW - there is no X509 certificate chain involved. Instead there is a pair of truncated SHA-1 hashes encrypted with UID-based key (the so called *Key 0x836*)

This prevents usage of NOR dumps from a different device than the one it originates from

## Building

Using `make`:

```
➜  Image1NORTool make
cc -arch x86_64 -arch arm64 -Iinclude -MMD -DPROJ_NAME=\"Image1NORTool\" main.c utils.c image1_nor.c -o build/Image1NORTool
```

## Usage

```
➜  Image1NORTool build/Image1NORTool 
Image1NORTool, made by @nyan_satan
usage: build/Image1NORTool VERB <options>
where VERB is one of the following:
        fix <input NOR> <output NOR> <key 0x836>
        make <raw input> <output Image1> <key 0x836>

only S5L8900X is supported for now
```

* `fix` verb accepts a NOR dump as an input and fixes the hashes in LLB's Image1 header. iPhone OS 1.1+ dumps, however, often have encrypted LLB payload. In such case you can first process the dump with original Key 0x836 (the tool also decrypts the payload), and only then with a key of your target device

* `make` verb accepts raw payload for input and outputs Image1. This is useful for ancient dumps that do not have Image1 containers at all (such as the famous Alpine 1A420)

## About Key 0x836

This is a result of encrypting `00E5A0E6526FAE66C5C1C6D4F16D6180` with device's UID-key

If there is any demand, I will release tools for generating this key, as well as for dumping/writing NORs
