# **CS670 Assignment 3: Secure Updates on User Matrix (Public Index) and Item Matrix (Private Index)**

| **Name**     | **Roll Number** | **Email**                                                 |
| ------------ | --------------- | --------------------------------------------------------- |
| Mahaarajan J | 220600          | [mahaarajan22@iitk.ac.in](mailto:mahaarajan22@iitk.ac.in) |

---

## **Objective**

This assignment implements a secure multiparty protocol for updating user and item latent matrices in a recommendation-style learning model. The update rule is applied jointly by two computing peers holding additive shares of both matrices. The distinguishing features are:

* The **user index** $i$ is **public**.
* The **item index** $j$ is **private** and is accessed using a **13-bit Distributed Point Function (DPF)**.
* All arithmetic is performed in the finite field
  $$
  \mathbb{F}_p,\qquad p = 2^{13}-1 = 8191.
  $$
* A trusted server $P_2$ supplies **Du–Atallah correlated randomness** enabling secure vector dot-products and secure scalar multiplications.
* Two parties $P_0$ and $P_1$ compute the update:
  $$
  u_i \leftarrow u_i + \delta, v_j,\qquad
  v_j \leftarrow v_j + \delta, u_i,
  $$
  where
  $$
  \delta = 1 - \langle u_i, v_j\rangle.
  $$

Correctness is verified offline by reconstructing shares and comparing against the mathematical update rule.

---

## **System Overview**

### **1. Query Generator**

`gen_queries.cpp` generates:

* A public user index $i$.
* A private item index $j$ encoded as a **pair of DPF keys** $(k_0, k_1)$.
* Two output files `queries_0.txt` and `queries_1.txt` containing the two DPF keys.

The keys are constructed so that:

1. **Their evaluated outputs form additive shares** of the one-hot vector $e_j$:
   $$
   e_j^{(0)} + e_j^{(1)} = e_j.
   $$

2. The **advise bit** embedded during key generation ensures correct application of the DPF’s sign convention:
   if the advise bit is $1$, the evaluator negates its share so that the final addition remains consistent in $\mathbb{F}_p$.

3. **The DPF flag bit is arranged such that the unique “flag = 1” at the target index is always placed in Key 1**, thereby enabling later FCW-based embedding of updates at the hidden index without ambiguity.

---

### **2. Trusted Server ($p2$)**

`p2.cpp` supplies all preprocessing randomness:

* $2K$ **$N$-dimensional Du–Atallah triples** for secure oblivious reads.
* $2K$ **scalar triples** for secure multiplications.
* One **$K$-dimensional Du–Atallah triple** for securely computing $\delta$.

Every triple is of the abstract form:
$$
(X, Y, \alpha), \qquad
\langle X, Y\rangle + \alpha = A\cdot B\ \text{in}\ \mathbb{F}_p,
$$
used to mask private multiplicands before exchanging them.

---

### **3. Peers $P_0$ and $P_1$**

Each peer:

1. Generates random additive shares of:
   $$
   U \in \mathbb{F}_p^{M\times K},\qquad
   V \in \mathbb{F}_p^{N\times K}.
   $$

2. Evaluates its DPF key locally to obtain shares of the one-hot vector $e_j$ **without ever learning $j$**.

3. Loads all correlated randomness sent by $P_2$.

4. Performs all online secure computations:
   oblivious read of $v_j$, computation of $\delta$, update of $u_i$, and secure column-wise update of $v_j$.

---

## **Mathematical Description of the Protocol**

### **Oblivious Read of the Item Vector**

For every coordinate $t\in{0,\ldots,K-1}$, each peer computes:
$$
v_j[t]^{(\ell)}
= \mathrm{MPCDOT}\bigl( V[:,t]^{(\ell)},\ e_j^{(\ell)} \bigr),
\qquad \ell\in{0,1}.
$$
Summing the shares yields the correct entry $v_j[t]$.

---

### **Secure Computation of $\delta$**

Peers use a $K$-dimensional Du–Atallah triple to securely compute:
$$
d = \langle u_i, v_j\rangle,
$$
and then derive:
$$
\delta = 1 - d.
$$
One peer offsets its share to ensure the additive reconstruction equals the expression above.

---

### **Secure Update of $u_i$**

For each coordinate $t$:
$$
u_i[t] \leftarrow u_i[t] + \delta, v_j[t].
$$
The product $\delta, v_j[t]$ is obtained via a scalar Du–Atallah multiplication triple.

---

## **Secure Update of the Item Profile**

The item update must embed $\delta,u_i[t]$ **only at the hidden row $j$** of $V$, without revealing the value of $j$ to either peer.
This is performed **column-wise**, using the DPF outputs and the structure of their flag bits.

Let:
$$
m_t = \delta \cdot u_i[t].
$$

For each column $t$, peers execute the following sequence:

### **1. DPF Evaluation Without Final Correction (Eval_all_indices_M)**

Each peer evaluates its DPF key using
`Eval_all_indices_M`, which returns two vectors:

* $\text{values}[i]$ — the raw evaluation value at index $i$ **before** the final correction word is applied,
* $\text{flags}[i]$ — a structural bit indicating whether the final correction word must be applied at index $i$.

The DPF construction ensures that:

* For all indices $i\neq j$, $\text{flags}[i]=0$,
* For the unique target index $j$, **only Party 0** has
  $$
  \text{flags}[j] = 1,
  $$
  where j is the target index

---

### **2. Construction of the Final Correction Word**
Party 1 negates all of its values (makes them MOD - val) then
Each peer computes:
$$
s_t = \sum_{i=0}^{N-1} \text{values}[i]- m_t
$$
and exchanges masked versions with the other peer and add them up


The **Final Correction Word (FCW)** for coordinate $t$ is then this sum for party 1 and the negation of this sum for party0. This is kept so that at the target index (where DPF values are unequal) party 0 will add it (M-$val_0[target]$+$val_1[target]$)

---

### **3. Column Replacement**

After correction, the updated column vector is written back:
$$
V[:,t]^{(\ell)} \leftarrow \text{values}^{(\ell)}.
$$

Performing this procedure for all $t\in{0,\ldots,K-1}$ yields the secure update:
$$
v_j \leftarrow v_j + \delta, u_i,
$$
with full privacy of the index $j$.

---

## **Correctness Checking**

`checker.cpp` reconstructs:

* Full matrices:
  $$
  U = U_0 + U_1,\qquad
  V = V_0 + V_1.
  $$
* Reconstructed $v_j$ shares,
* $\delta$,
* Updated profiles.

It verifies that for each query:
$$
u_i^{\mathrm{new}} = u_i + \delta,v_j,
\qquad
v_j^{\mathrm{new}} = v_j + \delta,u_i.
$$

If all validations succeed, the checker prints:

```
All queries processed.
```

---

## **Functions Implemented**

* Modular arithmetic:
  `add_mod`, `sub_mod`, `mul_mod`
* Vector operations:
  `dot_vector`, `sum_vector`, `col_extract`, `col_update`
* Random data generation:
  `generate_vector`, `generate_matrix`, `generate_int`
* DPF utilities:
  `generateDPF`,
  `Eval_all_indices` (DPF evaluation for basis vector),
  `Eval_all_indices_M` (DPF evaluation returning raw values and flags **without** final correction),
  `string_int_vector`
* MPC routines:
  `MPCDOT` — secure vector–vector dot product
  `MPC` — secure scalar multiplication
  `ex_values` — masked exchange for FCW reconstruction

---

## **Assumptions**

* The trusted server correctly generates Du–Atallah triples.
* DPF evaluation complies with the 13-bit field structure and produces valid additive shares of $e_j$.
* Channels between peers and the server are reliable.
* $(N,M,K,Q)$ remain small enough to store all offline randomness (< 200 recommended).

---

# **How to Run**

## **Automated Execution**

```
python3 test.py <num_runs>
```

This command:

1. Performs Docker pruning,
2. Builds all services,
3. Launches `gen_queries`, `p2`, `p0`, `p1`,
4. Runs the offline checker.

---

## **Manual Execution**

1. Edit `.env`:

```
N=...
M=...
K=...
Q=...
```

2. Run the system:

```
docker container prune -f
docker-compose up --build
```

3. After termination, run the checker:

```
g++ checker.cpp -o check
./check
```

Expected output:

```
All queries processed.
```
## **Declaration**

I hereby declare that this assignment is my individual and original work.
The implementation is original, built using C++ and Python, and adheres to the IIT Kanpur Academic Integrity Policy.
All code can be explained during evaluation.
