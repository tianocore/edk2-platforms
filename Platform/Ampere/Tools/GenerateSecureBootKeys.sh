#!/usr/bin/env bash

##
# @file
#  Script to download and generate keys/certificates/information
#  for Secure Boot.
#
# Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

set -o errexit

cleanup () {
  rm keys/dbx.priv >/dev/null 2>&1           || true
  rm keys/intermediate.priv >/dev/null 2>&1  || true
  rm keys/user.priv >/dev/null 2>&1          || true
  rm certs/??.pem >/dev/null 2>&1            || true
  rm certs/user.pfx >/dev/null 2>&1          || true
  rm certs/root.crt >/dev/null 2>&1          || true
  rm certs/intermediate.csr >/dev/null 2>&1  || true
  rm certs/intermediate.crt >/dev/null 2>&1  || true
  rm certs/user.csr >/dev/null 2>&1          || true
  rm certs/user.crt >/dev/null 2>&1          || true
  rm serial serial.* index.* >/dev/null 2>&1 || true
}

if [ -z "${CERT_PASSWORD}" ]; then
  CERT_PASSWORD=password
fi

if [ -z "${SECUREBOOT_DIR}" ]; then
  SECUREBOOT_DIR="${PWD}/secureboot_objects/"
fi

if [ ! -d "${SECUREBOOT_DIR}" ]; then
  mkdir "${SECUREBOOT_DIR}"
fi

pushd "${SECUREBOOT_DIR}" || exit 1

if [ -z "${USE_EXISTING_SB_KEYS}" ]; then
    mkdir keys   || true
    mkdir certs  || true
    cleanup

    OPENSSL_VERSION=$(openssl version | awk '{print $2}' | awk -F '.' '{print $1}')
    echo "OPENSSL VERSION $OPENSSL_VERSION"

    if [ "${OPENSSL_VERSION}" = "1" ]; then
      OPENSSL_CNF_FILE="openssl-1.cnf"
    else
      OPENSSL_CNF_FILE="openssl-3.cnf"
    fi

    if [ ! -f "${OPENSSL_CNF_FILE}" ]; then
      cp -vf "${WORKSPACE}/edk2-platforms/Platform/Ampere/Tools/${OPENSSL_CNF_FILE}" .
    fi

    rm -f openssl.cnf || true
    ln -isv ${OPENSSL_CNF_FILE} openssl.cnf

    echo "unique_subject = no" > index.txt.attr
    openssl req -config openssl.cnf -new -x509 -newkey rsa:2048 -subj "/CN=${BOARD_NAME} Platform Key/" -keyout keys/platform_key.priv -outform DER -out certs/platform_key.der -days 7300 -nodes -sha256
    openssl x509 -inform DER -in certs/platform_key.der -outform PEM -out certs/platform_key.pem
    openssl req -config openssl.cnf -new -x509 -newkey rsa:2048 -subj "/CN=${BOARD_NAME} Update Key/" -keyout keys/update_key.priv -outform DER -out certs/update_key.der -days 7300 -nodes -sha256
    openssl x509 -inform DER -in certs/update_key.der -outform PEM -out certs/update_key.pem

    # Root Certificate
    openssl req -config openssl.cnf -batch -new -x509 -days 3650 -key keys/update_key.priv -out certs/root.crt
    openssl x509 -in certs/root.crt -out certs/root.der -outform DER
    openssl x509 -inform DER -in certs/root.der -outform PEM -out certs/root.pub.pem

    # Intermediate Certificate
    openssl genrsa -aes256 -out keys/intermediate.priv -passout pass:"${CERT_PASSWORD}" 2048
    openssl req -config openssl.cnf -batch -new -key keys/intermediate.priv -out certs/intermediate.csr -passin pass:"${CERT_PASSWORD}" -passout pass:"${CERT_PASSWORD}"

    truncate -s0 index.txt
    echo 01 > serial

    openssl ca -config openssl.cnf -batch -extensions v3_ca -in certs/intermediate.csr -days 3650 -out certs/intermediate.crt -cert certs/root.crt -keyfile keys/update_key.priv
    openssl x509 -in certs/intermediate.crt -out certs/intermediate.der -outform DER
    openssl x509 -inform DER -in certs/intermediate.der -outform PEM -out certs/intermediate.pub.pem

    # User Certificate
    openssl genrsa -aes256 -out keys/user.priv -passout pass:"${CERT_PASSWORD}" 2048
    openssl req -config openssl.cnf -batch -new -key keys/user.priv -out certs/user.csr -passin pass:"${CERT_PASSWORD}" -passout pass:"${CERT_PASSWORD}"
    openssl ca -config openssl.cnf -batch -in certs/user.csr -days 3650 -out certs/user.crt -cert certs/intermediate.crt -keyfile keys/intermediate.priv -passin pass:"${CERT_PASSWORD}"
    openssl x509 -in certs/user.crt -out certs/user.der -outform DER
    openssl x509 -inform DER -in certs/user.der -outform PEM -out certs/user.pub.pem

    openssl pkcs12 -export -out certs/user.pfx -inkey keys/user.priv -in certs/user.crt -passin pass:"${CERT_PASSWORD}" -passout pass:"${CERT_PASSWORD}"
    openssl pkcs12 -in certs/user.pfx -nodes -out certs/user.pem -passin pass:"${CERT_PASSWORD}"
fi

python3 ${WORKSPACE}/edk2/BaseTools/Scripts/BinToPcd.py -i certs/root.der -p gEfiSecurityPkgTokenSpaceGuid.PcdPkcs7CertBuffer -o ${WORKSPACE}/edk2-platforms/Platform/${MANUFACTURER}/${BOARD_NAME}Pkg/root.cer.gEfiSecurityPkgTokenSpaceGuid.PcdPkcs7CertBuffer.inc

pushd certs
if [ ! -f "ms_kek1.der" ] || [ -n "${DOWNLOAD_MS_SB_KEYS}" ]; then
  curl -L "https://go.microsoft.com/fwlink/?LinkId=321185"  -o ms_kek1.der
fi
if [ ! -f "ms_kek2.der" ] || [ -n "${DOWNLOAD_MS_SB_KEYS}" ]; then
  curl -L "https://go.microsoft.com/fwlink/?linkid=2239775" -o ms_kek2.der
fi
if [ ! -f "ms_db1.der" ] || [ -n "${DOWNLOAD_MS_SB_KEYS}" ]; then
  curl -L "https://go.microsoft.com/fwlink/?linkid=321192"  -o ms_db1.der
fi
if [ ! -f "ms_db2.der" ] || [ -n "${DOWNLOAD_MS_SB_KEYS}" ]; then
  curl -L "https://go.microsoft.com/fwlink/?linkid=321194"  -o ms_db2.der
fi
if [ ! -f "ms_db3.der" ] || [ -n "${DOWNLOAD_MS_SB_KEYS}" ]; then
  curl -L "https://go.microsoft.com/fwlink/?linkid=2239776" -o ms_db3.der
fi
if [ ! -f "ms_db4.der" ] || [ -n "${DOWNLOAD_MS_SB_KEYS}" ]; then
  curl -L "https://go.microsoft.com/fwlink/?linkid=2239872" -o ms_db4.der
fi
if [ ! -f "ms_db5.der" ] || [ -n "${DOWNLOAD_MS_SB_KEYS}" ]; then
  curl -L "https://go.microsoft.com/fwlink/?linkid=2284009" -o ms_db5.der
fi
popd || exit 1

if [ ! -f "certs/dummy_dbx.der" ]; then
  # Generate a random certificate to place in the DBX. Otherwise, Linux won't try and update
  # the dbx variable when running `fwupgmgr`.
  openssl req -config openssl.cnf -new -x509 -newkey rsa:2048 -subj "/CN=Dummy DBX/" -keyout keys/dbx.priv -outform DER -out certs/dummy_dbx.der -days 7300 -nodes -sha256
fi

cleanup

popd || exit 1
