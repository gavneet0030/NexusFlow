import json
from pathlib import Path

import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score, roc_auc_score
from sklearn.model_selection import train_test_split
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler


SEED = 42
N = 20000

rng = np.random.default_rng(SEED)

amount = rng.lognormal(
    mean=7.0,
    sigma=1.0,
    size=N
)

priority = rng.integers(0, 4, size=N)

is_fraud_type = rng.binomial(1, 0.08, size=N)
is_suspicious_type = rng.binomial(1, 0.12, size=N)
is_chargeback_type = rng.binomial(1, 0.06, size=N)

high_value = (amount >= 10000).astype(int)
critical_priority = (priority == 3).astype(int)

X = np.column_stack([
    amount,
    priority,
    is_fraud_type,
    is_suspicious_type,
    is_chargeback_type,
    high_value,
    critical_priority,
])

logit = (
    -4.0
    + 0.00018 * amount
    + 0.45 * priority
    + 2.8 * is_fraud_type
    + 1.8 * is_suspicious_type
    + 2.2 * is_chargeback_type
    + 0.9 * high_value
    + 0.8 * critical_priority
)

probability = 1.0 / (
    1.0 + np.exp(-np.clip(logit, -30, 30))
)

y = rng.binomial(1, probability)

X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.20,
    random_state=SEED,
    stratify=y,
)

model = Pipeline([
    (
        "scaler",
        StandardScaler()
    ),
    (
        "classifier",
        LogisticRegression(
            random_state=SEED,
            max_iter=1000
        )
    ),
])

model.fit(X_train, y_train)

pred = model.predict(X_test)
pred_prob = model.predict_proba(X_test)[:, 1]

accuracy = accuracy_score(y_test, pred)
roc_auc = roc_auc_score(y_test, pred_prob)

scaler = model.named_steps["scaler"]
classifier = model.named_steps["classifier"]

artifact = {
    "model_type": "logistic_regression",

    "feature_names": [
        "amount",
        "priority",
        "is_fraud_type",
        "is_suspicious_type",
        "is_chargeback_type",
        "high_value",
        "critical_priority",
    ],

    "mean": scaler.mean_.tolist(),
    "scale": scaler.scale_.tolist(),
    "weights": classifier.coef_[0].tolist(),
    "intercept": float(classifier.intercept_[0]),

    "metrics": {
        "accuracy": float(accuracy),
        "roc_auc": float(roc_auc),
        "train_samples": int(len(X_train)),
        "test_samples": int(len(X_test)),
    },

    "training": {
        "seed": SEED,
        "samples": N,
        "algorithm": "LogisticRegression",
        "standardization": True,
    },
}

output = Path("ml/models/risk_model.json")

output.write_text(
    json.dumps(
        artifact,
        indent=2
    ),
    encoding="utf-8",
)

print("ML TRAINING: PASS")
print(f"TRAIN SAMPLES: {len(X_train)}")
print(f"TEST SAMPLES: {len(X_test)}")
print(f"ACCURACY: {accuracy:.6f}")
print(f"ROC_AUC: {roc_auc:.6f}")
print("MODEL EXPORT: PASS")
print(f"MODEL FILE: {output}")
