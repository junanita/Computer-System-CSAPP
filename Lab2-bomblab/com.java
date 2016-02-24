import java.util.*;
class com {

	List<List<Integer>> comb(int[] A, int k, int targetSum) {
		List<List<Integer>> result = new ArrayList<List<Integer>>();
		helper(A, 0, k, targetSum, new ArrayList<Integer>(), result);
		return result;

	}

	void helper(int[] A, int index, int k, int sum,
							List<Integer> entry, List<List<Integer>> result) {
		if (k == 0) {
			if (sum == 0) {
				result.add(new ArrayList<Integer>(entry));
			}
			return;
		}

		for (int i = index; i < A.length; i++) {
			if (sum - A[i] < 0) continue;
			entry.add(A[i]);
			helper(A, i+1, k - 1, sum - A[i], entry, result);
			entry.remove(entry.size() - 1);
		}

	}


	public static void main(String[] args) {
		int[] A = {2, 10, 6, 1, 12, 16, 9, 3, 4, 7};
		com b = new com();
		System.out.println(b.comb(A, 6, 50));

	}


}