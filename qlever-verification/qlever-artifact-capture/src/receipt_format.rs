//! CBOR receipt format schema definitions
//!
//! This module defines the canonical CBOR schema for verification receipts
//! as specified in EPIC 11 Part IV.

use serde::{Deserialize, Serialize};

/// CBOR schema version
pub const CBOR_SCHEMA_VERSION: u32 = 1;

/// Receipt format metadata
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReceiptFormatMeta {
    pub schema_version: u32,
    pub encoding: String,
    pub compression: Option<String>,
}

impl Default for ReceiptFormatMeta {
    fn default() -> Self {
        Self {
            schema_version: CBOR_SCHEMA_VERSION,
            encoding: "cbor".to_string(),
            compression: None,
        }
    }
}

/// Evidence entry format
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EvidenceEntry {
    pub key: String,
    pub data_type: EvidenceDataType,
    pub data: Vec<u8>,
    pub hash: String,
}

/// Evidence data types
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum EvidenceDataType {
    /// Raw bytes
    Bytes,
    /// UTF-8 text
    Text,
    /// JSON object
    Json,
    /// CBOR object
    Cbor,
    /// Hex-encoded digest
    Digest,
}

/// Validate that a receipt conforms to the CBOR schema
pub fn validate_receipt_schema(cbor_bytes: &[u8]) -> Result<(), SchemaValidationError> {
    // Try to deserialize as VerificationReceipt
    let _receipt: crate::VerificationReceipt = ciborium::from_reader(cbor_bytes)
        .map_err(|e| SchemaValidationError::DeserializationFailed(e.to_string()))?;

    // Schema is valid if deserialization succeeds
    Ok(())
}

/// Schema validation errors
#[derive(Debug, thiserror::Error)]
pub enum SchemaValidationError {
    #[error("Failed to deserialize CBOR: {0}")]
    DeserializationFailed(String),

    #[error("Missing required field: {0}")]
    MissingField(String),

    #[error("Invalid field type: {0}")]
    InvalidFieldType(String),
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{FailureClass, VerificationReceipt};

    #[test]
    fn test_schema_validation() {
        let receipt = VerificationReceipt::new(
            FailureClass::ReplayDivergence,
            "test".to_string(),
            "test".to_string(),
        );

        let mut cbor_bytes = Vec::new();
        ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();

        assert!(validate_receipt_schema(&cbor_bytes).is_ok());
    }

    #[test]
    fn test_invalid_schema() {
        let invalid_cbor = vec![0x00, 0x01, 0x02]; // Not valid CBOR for a receipt
        assert!(validate_receipt_schema(&invalid_cbor).is_err());
    }
}
